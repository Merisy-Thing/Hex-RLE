#include "dialog.h"
#include "ui_dialog.h"
#include <QString>
#include <QMessageBox>
#include <QRadioButton>
#include <QFont>
#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QByteArray>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    hex_uncompressed_len = 0;
    hex_compressed_len = 0;
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_btn_help_msg_clicked()
{
    const QString help_msg = tr(
    "======\r\n混合模式：每一组编码占两个字节字节或三个字节，由一个标志字节和一个或两个数据字节组成，如：\r\n\t0x96,0xAA,0x74,0xBB,0xCC;\r\n"
    "    如果标记字节的最高位为'1'时，数据字节为一个字节，编码模式同长模式，能表示的数据长度为127个字节(0x7F);\r\n"
    "    如果标记字节的最高位为'0'时，数据字节为两个个字节，编码模式同短模式，能表示的数据长度为7个字节(高3位)和16个字节(低4位)，"
    "如上例中原始数据为：\r\n\t0xAA,0xAA,......0xAA,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xCC,0xCC,0xCC,0xCC\r\n\t +- 22(0x16)个0xAA -+\r\n"
    "======\r\n短模式：每一组编码占三个字节，由一个标志字节和两个数据字节组成，如：\r\n\t0x23,0xAA,0xBB;\r\n"
    "    第一个字节的高低4位分别表示后面两个字节的重复长度，如上例中原始数据为：\r\n\t0xAA,0xAA,0xBB,0xBB,0xBB\r\n"
    "    短模式单次可编码的数据连续最大长度是16字节，超过16个的长度需要使用一个组或分组\r\n"
    "======\r\n长模式：每一组编码占两个字节，由一个标志字节和一个数据字节组成，如：\r\n\t0x03,0xAA,0x04,0xBB;\r\n"
    "    第一个字节表示第二个字节的重复长度，如上例中原始数据为：\r\n\t0xAA,0xAA,0xAA,0xBB,0xBB,0xBB,0xBB\r\n"
    "    长模式单次可编码的数据连续最大长度是255字节，超过255个的长度需要分组\r\n"
    );

    ui->textEdit_after->clear();
    help_msg.toUtf8();
    ui->textEdit_after->setText(help_msg);
}

unsigned char Dialog::hex_to_bin(QChar hex)
{
    unsigned char ascii_hex = hex.toAscii();

    if((ascii_hex >= '0') && (ascii_hex <= '9'))
        return ascii_hex - '0';
    if((ascii_hex >= 'A') && (ascii_hex <= 'F'))
        return ascii_hex - 'A' + 10;
    if((ascii_hex >= 'a') && (ascii_hex <= 'f'))
        return ascii_hex - 'a' + 10;
    return 0;
}

bool Dialog::get_uncompressed_hex()
{
    QString str_text;
    int str_len = 0;

    hex_uncompressed_len = 0;

    str_text = ui->textEdit_before->toPlainText();
    str_len = str_text.length();

    for(int i=0; i<str_len-3; i++) {
        if((str_text.at(i) == '0') && (str_text.at(i+1) == 'x')) {
            hex_uncompressed[hex_uncompressed_len] = (hex_to_bin(str_text.at(i+2))<<4) + hex_to_bin(str_text.at(i+3));
            if(++hex_uncompressed_len >= HEX_UNCOMPRESSED_BUFFER_SIZE) {
                return false;
            }
        }
    }
    if(hex_uncompressed_len == 0) {
        return false;
    } else {
        return true;
    }
}

int Dialog::get_data_repeat_length(const unsigned char *buffer, int buffer_len)
{
    int repeat;
    unsigned char c;

    if((buffer == NULL) || (buffer_len <= 0)) {
        return 0;
    }
    if(buffer_len == 1) {
        return 1;
    }

    repeat = 1;
    c = buffer[0];
    for(int i=1; i<buffer_len; i++) {
        if(c == buffer[i]) {
            repeat++;
        } else {
            return repeat;
        }
    }
    return buffer_len;
}

//Mix RLE Format exsample:
bool Dialog::rle_compress_mix_mode(const unsigned char *in_buffer, int in_buffer_len, unsigned char *out_buffer, int *out_buffer_len)
{
    int out_index = 0, in_index = 0, repeat_len, repeat_len_next;

    if((in_buffer == NULL) || (out_buffer == NULL) || (in_buffer_len <= 0) || (*out_buffer_len < 3)) {
        return false;
    }
    while(in_index < in_buffer_len) {
        repeat_len = get_data_repeat_length(&in_buffer[in_index], in_buffer_len - in_index);
        if(repeat_len == 0) {
            return false;
        }
        if(repeat_len > 7) {//Like long mode
            if(repeat_len > 0x7F) {//  >127
                out_buffer[out_index++] = 0x7F + 0x80;//0x80: Add long mode mask
                out_buffer[out_index++] = in_buffer[in_index];
                in_index += 0x7F;
            } else {
                out_buffer[out_index++] = repeat_len + 0x80;//0x80: Add long mode mask
                out_buffer[out_index++] = in_buffer[in_index];
                in_index += repeat_len;
            }
        } else {
            repeat_len_next = get_data_repeat_length(&in_buffer[in_index + repeat_len], in_buffer_len - in_index - repeat_len);
            if(repeat_len_next == 0) {
                out_buffer[out_index++] = repeat_len + 0x80;//0x80: Add long mode mask
                out_buffer[out_index++] = in_buffer[in_index];
                *out_buffer_len = out_index;
                return true;
            }
            if(repeat_len_next < 16) {
                out_buffer[out_index++] = ((repeat_len<<4)&0x70) + repeat_len_next;
                out_buffer[out_index++] = in_buffer[in_index];
                out_buffer[out_index++] = in_buffer[in_index + repeat_len];
                in_index += repeat_len + repeat_len_next;
            } else {
                out_buffer[out_index++] = ((repeat_len<<4)&0x70) + 0x0F;
                out_buffer[out_index++] = in_buffer[in_index];
                out_buffer[out_index++] = in_buffer[in_index + repeat_len];
                in_index += repeat_len + 0x0F;
            }
        }
    }

    *out_buffer_len = out_index;
    return ((out_index == 0)?false:true);
}
//Format exsample:
//char *input = "WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW";
//char *output = "12W1B12W3B24W1B14W";
bool Dialog::rle_compress_long_mode(const unsigned char *in_buffer, int in_buffer_len, unsigned char *out_buffer, int *out_buffer_len)
{
    int i, out_index = 0;
    unsigned char prev = 0, curr = 0, repeat = 1;

    if((in_buffer == NULL) || (out_buffer == NULL) || (in_buffer_len <= 0) || (*out_buffer_len < 2)) {
        return false;
    }

    prev = in_buffer[0];
    for(i=1; i<in_buffer_len; i++){
        curr = in_buffer[i];
        if(prev == curr) {
            if(repeat < 0xFF) {
                repeat++;
            } else  {
                repeat = 1;
                out_buffer[out_index++] = 0xFF;
                out_buffer[out_index++] = prev;
            }
        } else {
            out_buffer[out_index++] = repeat;
            out_buffer[out_index++] = prev;
            prev = curr;
            repeat = 1;
        }
        if(out_index >= *out_buffer_len) {
            return false;
        }
    }
    out_buffer[out_index++] = repeat;
    out_buffer[out_index++] = prev;

    *out_buffer_len = out_index;
    return true;
}
//Format exsample:
//char *input =
//char *output =
bool Dialog::rle_compress_short_mode(const unsigned char *in_buffer, int in_buffer_len, unsigned char *out_buffer, int *out_buffer_len)
{
    int out_index = 0, in_index = 0, repeat_len;

    if((in_buffer == NULL) || (out_buffer == NULL) || (in_buffer_len <= 0) || (*out_buffer_len < 3)) {
        return false;
    }
    while(in_index < in_buffer_len) {
        repeat_len = get_data_repeat_length(&in_buffer[in_index], in_buffer_len - in_index);
        if(repeat_len == 0) {
            return false;
        }
        if(repeat_len > 15) {//Like long mode
            out_buffer[out_index++] = 0xF0;
            out_buffer[out_index++] = in_buffer[in_index];
            in_index += 15;
        } else {
            out_buffer[out_index++] = (repeat_len<<4)&0xF0;
            out_buffer[out_index++] = in_buffer[in_index];
            in_index += repeat_len;
        }

        repeat_len = get_data_repeat_length(&in_buffer[in_index], in_buffer_len - in_index);
        if(repeat_len == 0) {
            *out_buffer_len = out_index;
            return true;
        }
        if(repeat_len > 15) {
            out_buffer[out_index-2] += 0x0F;
            out_buffer[out_index++] = in_buffer[in_index];
            in_index += 15;
        } else {
            out_buffer[out_index-2] += repeat_len;
            out_buffer[out_index++] = in_buffer[in_index];
            in_index += repeat_len;
        }
    }

    *out_buffer_len = out_index;
    return ((out_index == 0)?false:true);
}
void Dialog::disp_after_data(unsigned char *after_buffer, int buffer_len)
{
    QString str;

    for(int i=0; i<buffer_len; i++) {
        str.sprintf("0x%02x,",after_buffer[i]);
        ui->textEdit_after->insertPlainText(str);
        if((i+1)%16 == 0) {
            ui->textEdit_after->insertPlainText(tr("\r\n"));
        }
    }
}

void Dialog::on_btn_compress_clicked()
{
    QMessageBox msgBox;
    QString str;

    ui->lb_compress_ratio->setText(tr("??%"));
    if(get_uncompressed_hex() != true) {
        msgBox.setText(tr(" Read source data fail... "));
        msgBox.exec();
        return;
    }

    hex_compressed_len = HEX_COMPRESSED_BUFFER_SIZE;
    if(ui->rB_long_mode->isChecked() == true) {
        if(rle_compress_long_mode(hex_uncompressed, hex_uncompressed_len, hex_compressed, &hex_compressed_len) != true) {
            msgBox.setText(tr("Long RLE compress fail... "));
            msgBox.exec();
            return;
        }
    } else if(ui->rB_short_mode->isChecked() == true) {
        if(rle_compress_short_mode(hex_uncompressed, hex_uncompressed_len, hex_compressed, &hex_compressed_len) != true) {
            msgBox.setText(tr("Short RLE compress fail... "));
            msgBox.exec();
            return;
        }
    } else if(ui->rB_mix_mode->isChecked() == true) {
        if(rle_compress_mix_mode(hex_uncompressed, hex_uncompressed_len, hex_compressed, &hex_compressed_len) != true) {
            msgBox.setText(tr("Mix RLE compress fail... "));
            msgBox.exec();
            return;
        }
    } else {
        msgBox.setText(tr("Radio button group check error ... "));
        msgBox.exec();
        return;
    }

    ui->textEdit_after->clear();
    disp_after_data(hex_compressed, hex_compressed_len);

    str.sprintf("%d Byte",hex_uncompressed_len);
    ui->lb_before_length->setText(str);

    str.sprintf("%d Byte",hex_compressed_len);
    ui->lb_after_length->setText(str);

    str.sprintf("%.3f%%",(float)(hex_compressed_len*100)/(float)hex_uncompressed_len);
    ui->lb_compress_ratio->setText(str);
    if((hex_compressed_len*100)/hex_uncompressed_len >= 100) {
        ui->lb_compress_ratio->setStyleSheet("QLabel { background-color : red; color : white; }");
    } else {
        ui->lb_compress_ratio->setStyleSheet("QLabel { background-color : white; color : black; }");
    }
}

void Dialog::on_btn_decompress_clicked()
{
    ui->textEdit_after->setPlainText(tr("功能示实现"));
}

void Dialog::on_btn_clear_clicked()
{
    ui->textEdit_before->clear();
    ui->textEdit_after->clear();
}

void Dialog::on_btn_save_to_bin_file_clicked()
{
    QMessageBox msgBox;
    QString file_name;
    QFile file;
    QByteArray byte_array;

    msgBox.setFont(QFont("Times",10,QFont::Normal));
    if(hex_compressed_len == 0){
        msgBox.setText(tr("没有数据可以存储..."));
        msgBox.exec();
        return;
    }

    file_name = QFileDialog::getSaveFileName(this, tr("Save File"),
                                "R:/", tr("Bin (*.bin);;All (*.*)"));
    if(file_name.isEmpty() == true) {
        return;
    }

    file.setFileName(file_name);
    if (!file.open(QIODevice::WriteOnly)) {
        msgBox.setText(tr("打开文件出错..."));
        msgBox.exec();
        return;
    }
    byte_array.setRawData((char *)hex_compressed, hex_compressed_len);
    if(file.write(byte_array) < 0) {
        msgBox.setText(tr("写文件出错..."));
        msgBox.exec();
    }

    file.close();
}






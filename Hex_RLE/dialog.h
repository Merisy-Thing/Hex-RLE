#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#define HEX_UNCOMPRESSED_BUFFER_SIZE    64*1024  //64KB
#define HEX_COMPRESSED_BUFFER_SIZE      128*1024 //128KB

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    /*variable*/
    unsigned char hex_uncompressed[HEX_UNCOMPRESSED_BUFFER_SIZE];
    int hex_uncompressed_len;
    unsigned char hex_compressed[HEX_COMPRESSED_BUFFER_SIZE];
    int hex_compressed_len;

    /*Function*/
    bool get_uncompressed_hex();
    unsigned char hex_to_bin(QChar hex);
    int get_data_repeat_length(const unsigned char *in_buffer, int in_buffer_len);
    bool rle_compress_long_mode(const unsigned char *in_buffer, int in_buffer_len, unsigned char *out_buffer, int *out_buffer_len);
    bool rle_compress_short_mode(const unsigned char *in_buffer, int in_buffer_len, unsigned char *out_buffer, int *out_buffer_len);
    bool rle_compress_mix_mode(const unsigned char *in_buffer, int in_buffer_len, unsigned char *out_buffer, int *out_buffer_len);

    void disp_after_data(unsigned char *after_buffer, int buffer_len);

private slots:
    void on_btn_save_to_bin_file_clicked();
    void on_btn_help_msg_clicked();
    void on_btn_clear_clicked();
    void on_btn_decompress_clicked();
    void on_btn_compress_clicked();
};

#endif // DIALOG_H

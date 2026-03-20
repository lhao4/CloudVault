#ifndef SHAREFILE_H
#define SHAREFILE_H

#include <QWidget>

namespace Ui {
class ShareFile;
}

class ShareFile : public QWidget
{
    Q_OBJECT

public:
    explicit ShareFile(QWidget *parent = nullptr);
    void updataFriend_LW();
    ~ShareFile();

private slots:
    void on_allSelect_PB_clicked();

    void on_cancleSelec_PB_clicked();

    void on_confirm_PB_clicked();

private:
    Ui::ShareFile *ui;
};

#endif // SHAREFILE_H

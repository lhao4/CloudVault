#include "index.h"
#include "ui_index.h"
#include "file.h"

Index::Index(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Index)
{
    ui->setupUi(this);
}

Index::~Index()
{
    delete ui;
}

Index& Index::getInstance()
{
    static Index instance;
    return instance;
}

Friend *Index::getFriend()
{
    return ui->friendPage;
}

File *Index::getFile()
{
    return ui->filePage;
}

void Index::on_friend_PB_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Index::on_file_PB_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

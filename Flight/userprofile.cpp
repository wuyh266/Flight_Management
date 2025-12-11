#include "userprofile.h"
#include "ui_userprofile.h"
#include "sign_in.h"
#include "edit_infor.h"
#include "passenger.h"
#include <QMessageBox>
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QPixmap>
#include<QFile>
UserProfile::UserProfile(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UserProfile)
{
    ui->setupUi(this);
    connect(ui->btn_back, &QPushButton::clicked, this, &UserProfile::on_btn_back_clicked);
    QFile qssFile(":/styles/userprofile.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}


UserProfile::UserProfile(const QString &userID, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UserProfile)
{
    ui->setupUi(this);

    this->userID = userID;
    connect(ui->btn_back, &QPushButton::clicked, this, &UserProfile::on_btn_back_clicked);
    if (!this->userID.isEmpty()) {
        getData(this->userID);
    }
    QFile qssFile(":/styles/userprofile.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}

UserProfile::~UserProfile()
{
    delete ui;
}

void UserProfile::on_btn_back_clicked()
{
    emit backRequested();
}

void UserProfile::on_pushButton_4_clicked() // 我的订单
{
    emit myOrdersRequested();
}

void UserProfile::on_pushButton_8_clicked() // 注册
{
    Sign_in *s = new Sign_in();
    s->show();
}


void UserProfile::on_pushButton_7_clicked() // 登录
{
    QMessageBox msgBox(QMessageBox::Question, "取消登录", "确定要退出登录吗？",
                       QMessageBox::Yes | QMessageBox::No, this);

    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: white;"
        "}"
        "QLabel {"
        "    color: black; "
        "}"
        "QPushButton {"
        "    min-width: 80px; "
        "   color:black;"
        "   background-color:white;"
        "   border: 1px solid black; "
        "}"
        );

    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        emit logoutRequested();
    }
}


void UserProfile::on_pushButton_9_clicked() // 注销账号
{
    QMessageBox msgBox(QMessageBox::Question, "注销账号",
                       "确定要永久注销您的账号吗？\n此操作将删除您的所有个人信息、订单记录及余额，且不可恢复！",
                       QMessageBox::Yes | QMessageBox::No, this);

    msgBox.setStyleSheet(
        "QMessageBox { background-color: white; }"
        "QLabel { color: black; }"
        "QPushButton { min-width: 80px; color:black; background-color:white; border: 1px solid black; }"
        );

    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        if (this->userID.isEmpty()) {
            QMessageBox::warning(this, "错误", "无法获取当前用户ID，注销失败。");
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            QMessageBox::warning(this, "错误", "数据库未连接。");
            return;
        }
        db.transaction();
        QSqlQuery query(db);

        try {
            // 删除用户的收藏
            query.prepare("DELETE FROM favorites WHERE UserID = ?");
            query.addBindValue(this->userID);
            if (!query.exec()) throw query.lastError();

            // 删除乘客信息
            query.prepare("DELETE FROM passengers WHERE UserID = ?");
            query.addBindValue(this->userID);
            if (!query.exec()) throw query.lastError();

            // 删除订单
            query.prepare("DELETE FROM orders WHERE UserID = ?");
            query.addBindValue(this->userID);
            if (!query.exec()) throw query.lastError();

            // 删除用户主表记录
            query.prepare("DELETE FROM users WHERE UserID = ?");
            query.addBindValue(this->userID);
            if (!query.exec()) throw query.lastError();

            // 提交事务
            db.commit();
            QMessageBox::information(this, "成功", "账号已成功注销，期待与您再会。");

            //发送退出信号，返回登录界面
            emit logoutRequested();

        } catch (const QSqlError &e) {
            db.rollback();
            QMessageBox::critical(this, "错误", "注销失败，数据库错误：" + e.text());
            qDebug() << "Delete user error:" << e.text();
        }
    }
}

void UserProfile::on_pushButton_10_clicked(){
    passenger*s=new passenger(currentUsername);
    s->show();
}

void UserProfile::on_pushButton_5_clicked()
{
    edit_infor *e = new edit_infor(this->userID, ui->txt_Username->text(), this);

    connect(e, &edit_infor::change_name, this, [this](QString old, QString name){
        if (name == ""){
            ui->txt_Username->setText(old);
            this->currentUsername = name;
        } else {
            ui->txt_Username->setText(name);
            this->currentUsername = old;
        }
    });

    connect(e, &edit_infor::change_jianjie, this, [this](QString old, QString jianjie){
        if (jianjie == ""){
            ui->txt_jianjie->setText(old);
        } else {
            ui->txt_jianjie->setText(jianjie);
        }
    });

    connect(e, &edit_infor::change_avatar, this, [this](QPixmap pixmap){
        ui->label_2->setPixmap(pixmap);
         emit avatarUpdated();
    });

    connect(e, &QDialog::finished, this, [this]{
        this->show();
        if (!this->userID.isEmpty()) {
            getData(this->userID);
        }
    });
    this->hide();
    e->exec();
}


void UserProfile::getData(const QString &userID)
{
    qDebug() << "getData called with userID:" << userID;
    if (userID.isEmpty()) return;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.setForwardOnly(true);

    query.prepare("SELECT Username, IDCard, jianjie, avatar, Balance FROM users WHERE UserID = ?");
    query.addBindValue(userID.toInt());

    if (query.exec()) {
        if (query.next()) {
            QString username = query.value(0).toString();
            QString idCard = query.value(1).toString();
            QString jianjie = query.value(2).toString();
            QByteArray avatarData = query.value(3).toByteArray();
            double balance = query.value(4).toDouble();
            ui->txt_Username->setText(username);
            this->currentUsername = username;
            ui->txt_UserAccount->setText(idCard);
            ui->txt_jianjie->setText(jianjie);
            ui->txt_yu->setText(QString::number(balance, 'f', 2));
            if (!avatarData.isEmpty()) {
                QPixmap pixmap;
                if (pixmap.loadFromData(avatarData)) {
                    ui->label_2->setPixmap(pixmap.scaled(ui->label_2->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    ui->label_2->clear();
                }
            } else {
                ui->label_2->clear();
            }
        }
        query.finish(); // 成功时关闭
    } else {
        qDebug() << "getData Error:" << query.lastError().text();
        query.finish();
    }
}
void UserProfile::on_btn_favorites_clicked()
{
    emit myFavoritesRequested();
}

void UserProfile::on_btn_recharge_clicked(){
    if (payWindow == nullptr) {
        payWindow = new Pay();

        // 当支付成功时，刷新个人中心数据
        connect(payWindow, &Pay::paymentSuccess, this, [this](){
            if (!this->userID.isEmpty()) {
                this->getData(this->userID); // 刷新余额显示
            }
        });

        connect(payWindow, &QWidget::destroyed, this, [=](){ payWindow = nullptr; });
    }
    payWindow->setUserID(this->userID);

    payWindow->show();
    payWindow->raise();
    payWindow->activateWindow();
}

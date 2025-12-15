#include "deal.h"
#include "ui_deal.h"
#include "single_center.h"
#include "order_dialog.h"
#include "userprofile.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QDate>
#include <QPushButton>
#include <QHeaderView>
#include <QDateTime>
#include <QTableWidgetItem>
#include <QElapsedTimer> // 新增：用于性能计时
#include <QScrollBar>    // 新增：控制滚动条
#include "mainwindow.h"
#include "userprofile.h"
#include<QFile>
#include<QPainter>
#include<QPainterPath>
#define PAGE_SIZE 50  // 每页显示50条
int currentPage = 1;
int totalPage = 0;

Deal::Deal(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Deal)
{
    ui->setupUi(this);
    currentUserID = "";
    initTable();
    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setMinimumDate(QDate::currentDate());
    ui->stackedWidget->setCurrentWidget(ui->page_tickets);
     QFile qssFile(":/styles/Dealstyle.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }

    initPagination();
}

Deal::Deal(const QString &userID, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Deal)
{
    ui->setupUi(this);
    currentUserID = userID;
    initTable();
    getData(userID);
    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setMinimumDate(QDate::currentDate());

    searchTickets(currentPage);

    m_personalCenterPage = new Single_Center(currentUserID, this);
    ui->stackedWidget->addWidget(m_personalCenterPage);
    connect(m_personalCenterPage, &Single_Center::backRequested, this, &Deal::showTicketSearchPage);

    m_userProfilePage = new UserProfile(userID,this);
    ui->stackedWidget->addWidget(m_userProfilePage);
    connect(m_userProfilePage, &UserProfile::backRequested, this, &Deal::showTicketSearchPage);

    // 创建收藏夹页面对象
    m_favoriteDialogPage = new favorite_dialog(currentUserID, this);

    // 将其添加到 stackedWidget (页面栈) 中
    ui->stackedWidget->addWidget(m_favoriteDialogPage);

    // 连接信号：从收藏夹点击“返回”时，回到票务搜索页
    connect(m_favoriteDialogPage, &favorite_dialog::backRequested, this, &Deal::showTicketSearchPage);

    //连接信号：从个人中心点击“我的收藏”时，跳转到收藏夹
    connect(m_userProfilePage, &UserProfile::myFavoritesRequested, this, [=](){
        m_favoriteDialogPage->refreshFavoriteList();
        ui->stackedWidget->setCurrentWidget(m_favoriteDialogPage);
    });

    connect(m_userProfilePage, &UserProfile::myOrdersRequested, this, [=](){
        m_personalCenterPage->refreshOrderList();
        ui->stackedWidget->setCurrentWidget(m_personalCenterPage);
    });
    connect(m_userProfilePage, &UserProfile::avatarUpdated, this, [=](){
        if (!currentUserID.isEmpty()) {
            getData(currentUserID);
        }
    });
    connect(m_userProfilePage, &UserProfile::logoutRequested, this, [=](){
        MainWindow *loginWindow = new MainWindow();
        loginWindow->show();
        this->close();
    });
    connect(m_personalCenterPage, &Single_Center::dataChanged, this, [=](){
        // 1. 刷新票务列表（更新座位数）
        this->searchTickets(currentPage);

        // 2. 刷新个人中心数据（更新余额）
        if (m_userProfilePage) {
            m_userProfilePage->getData(currentUserID);
        }
    });
    ui->stackedWidget->setCurrentWidget(ui->page_tickets);

    initPagination();
    //点击头像进入个人界面
    connect(ui->Avatar, &QPushButton::clicked, this, [=](){
        if (currentUserID.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先登录！");
            return;
        }
        m_userProfilePage->getData(currentUserID);
        ui->stackedWidget->setCurrentWidget(m_userProfilePage);
    });
    ui->stackedWidget->setCurrentWidget(ui->page_tickets);
     QFile qssFile(":/styles/Dealstyle.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(qssFile.readAll());
        this->setStyleSheet(styleSheet);  // 只影响当前窗口
        qssFile.close();
        qDebug()<<"成功读取文件";
    }
}
//绘制圆形头像
void Deal::setCircularAvatar(const QByteArray &avatarData)
{
    QPixmap pixmap;
    if (!avatarData.isEmpty() && pixmap.loadFromData(avatarData)) {
        // 创建40x40的圆形头像
        QPixmap circularPixmap(40, 40);
        circularPixmap.fill(Qt::transparent);

        QPainter painter(&circularPixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // 创建圆形路径
        QPainterPath path;
        path.addEllipse(0, 0, 40, 40);
        painter.setClipPath(path);

        // 缩放并绘制图片
        QPixmap scaled = pixmap.scaled(40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        painter.drawPixmap(0, 0, scaled);

        // 绘制白色边框（可选）
        painter.setPen(QPen(QColor(255, 255, 255, 153), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(0, 0, 39, 39);  // 40x40 所以是 39

        ui->Avatar->setIcon(QIcon(circularPixmap));
        ui->Avatar->setIconSize(QSize(40, 40));
    }
}
//获取头像信息
void Deal::getData(const QString &userID)
{
    qDebug() << "getData called with userID:" << userID;
    if (userID.isEmpty()) return;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.setForwardOnly(true);

    query.prepare("SELECT  avatar FROM users WHERE UserID = ?");
    query.addBindValue(userID.toInt());

    if (query.exec()) {
        if (query.next()) {

            QByteArray avatarData = query.value(0).toByteArray();

            if (!avatarData.isEmpty()) {
                setCircularAvatar(avatarData);
            }
        }
        query.finish(); // 成功时关闭
    } else {
        qDebug() << "getData Error:" << query.lastError().text();
        query.finish();
    }
}
Deal::~Deal()
{
    delete ui;
}


void Deal::initTable()
{
    QStringList headers;
    headers << "编号" << "出发地" << "目的地" << "出发时间"
            << "到达时间" << "价格(元)" << "可用座位" << "公司" << "操作" << "收藏";
    ui->tableWidget_tickets->setColumnCount(headers.size());
    ui->tableWidget_tickets->setHorizontalHeaderLabels(headers);
    // 关键列手动调整宽度（避免文字截断）
    ui->tableWidget_tickets->setColumnWidth(1, 155);  //出发地
    ui->tableWidget_tickets->setColumnWidth(2, 155);  //目的地
    ui->tableWidget_tickets->setColumnWidth(3, 130);  // 出发时间
    ui->tableWidget_tickets->setColumnWidth(4, 130);  // 到达时间
    ui->tableWidget_tickets->setColumnWidth(5, 85);   // 价格(元)
    ui->tableWidget_tickets->setColumnWidth(6, 80);   //可用座位
    ui->tableWidget_tickets->setColumnWidth(7, 90);   //公司
    ui->tableWidget_tickets->setColumnWidth(8, 55);   // 操作列
    ui->tableWidget_tickets->setColumnWidth(9, 55);   //收藏
    ui->tableWidget_tickets->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_tickets->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_tickets->verticalHeader()->setVisible(false);
    ui->tableWidget_tickets->setUpdatesEnabled(false);
}

int Deal::getTotalPage()
{
    QString from = ui->lineEdit_from->text().trimmed();
    QString to = ui->lineEdit_to->text().trimmed();
    QString startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString searchTime=ui->dateEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss");

    if (!QSqlDatabase::database().isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未连接！");
        return 0;
    }
    QList<int> favoriteTicketIds;
    if (!currentUserID.isEmpty()) {
        QSqlQuery favQuery;
        favQuery.prepare("SELECT TicketID FROM favorites WHERE UserID = :uid");
        favQuery.bindValue(":uid", currentUserID);
        if (favQuery.exec()) {
            while(favQuery.next()) {
                favoriteTicketIds.append(favQuery.value(0).toInt());
            }
        }
        favQuery.finish();
    }

    // 检查数据库连接
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        return 0;
    }

    // 查询符合条件的总条数
    QString countSql = "SELECT COUNT(*) FROM flight_info WHERE status = 'On Time' "
                       "AND departure_time >= :start_time ";
    if (!from.isEmpty()) {
        countSql += "AND departure_city LIKE :from ";
    }
    if (!to.isEmpty()) {
        countSql += "AND arrival_city LIKE :to ";
    }

    QSqlQuery countQuery(db);
    countQuery.prepare(countSql);
    if(searchTime>startTime)
        countQuery.bindValue(":start_time", searchTime);
    else
        countQuery.bindValue(":start_time", startTime);
    if (!from.isEmpty()) {
        countQuery.bindValue(":from", from + "%");
    }
    if (!to.isEmpty()) {
        countQuery.bindValue(":to", to + "%");
    }

    int totalCount = 0;
    if (countQuery.exec()) {
        // 检查是否有结果
        if (countQuery.next()) {
            totalCount = countQuery.value(0).toInt();
        }

        countQuery.finish();
    } else {
        qDebug() << "查询总条数失败：" << countQuery.lastError().text();

        countQuery.finish();
        return 0;
    }

    // 计算总页数（向上取整）
    if (totalCount == 0) {
        return 0;
    }
    return (totalCount + PAGE_SIZE - 1) / PAGE_SIZE;
}

void Deal::initPagination()
{
    ui->lineEdit_pageNum->setAlignment(Qt::AlignCenter);

    QIntValidator *validator = new QIntValidator(1, 999, this);
    ui->lineEdit_pageNum->setValidator(validator);
    ui->lineEdit_pageNum->setAlignment(Qt::AlignCenter);

    ui->btn_prev->setIcon(QIcon(":/img/LeftArrow.png"));
    ui->btn_prev->setIconSize(QSize(20, 20));
    ui->btn_prev->setFlat(true);
    ui->btn_prev->setCursor(Qt::PointingHandCursor);

    // 下一页按钮
    ui->btn_next->setIcon(QIcon(":/img/rightArrow.png"));
    ui->btn_next->setIconSize(QSize(20, 20));
    ui->btn_next->setFlat(true);
    ui->btn_next->setCursor(Qt::PointingHandCursor);

    disconnect(ui->btn_prev, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->btn_prev, &QPushButton::clicked, this, [=](){
        if (currentPage > 1) {
            currentPage--;
            searchTickets(currentPage);
        }
    });

    // 下一页按钮逻辑
    disconnect(ui->btn_next, &QPushButton::clicked, nullptr, nullptr);
    connect(ui->btn_next, &QPushButton::clicked, this, [=](){
        if (currentPage < totalPage) {
            currentPage++;
            searchTickets(currentPage);
        }
    });

    // 页码回车跳转
    disconnect(ui->lineEdit_pageNum, &QLineEdit::returnPressed, nullptr, nullptr);
    connect(ui->lineEdit_pageNum, &QLineEdit::returnPressed, this, &Deal::on_lineEdit_pageNum_returnPressed);

    //初始化状态
    updatePageContainerText();
}


void Deal::updatePageContainerText()
{
    if (totalPage == 0) {
        ui->label_pageInfo->setText("  页 / 共 0 页");
        ui->lineEdit_pageNum->setEnabled(false);
        ui->lineEdit_pageNum->setText("");

        // 禁用按钮
        ui->btn_prev->setEnabled(false);
        ui->btn_next->setEnabled(false);
        return;
    }

    // 有数据时的处理
    QString pageText = QString("  页 / 共 %1 页").arg(totalPage);
    ui->label_pageInfo->setText(pageText);


    ui->lineEdit_pageNum->setEnabled(true);
    ui->lineEdit_pageNum->setText(QString::number(currentPage));
    ui->lineEdit_pageNum->raise(); // 确保输入框在最上层

    // 更新按钮状态
    // 如果是第一页，禁用“上一页”；如果是最后一页，禁用“下一页”
    ui->btn_prev->setEnabled(currentPage > 1);
    ui->btn_next->setEnabled(currentPage < totalPage);
}
void Deal::on_lineEdit_pageNum_returnPressed()
{
    // 无数据时直接返回
    if (totalPage == 0) {
        QMessageBox::warning(this, "提示", "暂无数据，无法跳转！");
        ui->lineEdit_pageNum->setText("");
        return;
    }

    // 获取输入并校验格式
    QString inputText = ui->lineEdit_pageNum->text().trimmed();
    if (inputText.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入有效页码！");
        ui->lineEdit_pageNum->setText(QString::number(currentPage)); // 恢复当前页
        return;
    }

    bool isNumber;
    int inputPage = inputText.toInt(&isNumber);
    if (!isNumber) {
        QMessageBox::warning(this, "提示", "页码必须是正整数！");
        ui->lineEdit_pageNum->setText(QString::number(currentPage)); // 恢复当前页
        return;
    }

    // 边界校验
    int targetPage = inputPage;
    if (targetPage < 1) {
        targetPage = 1;
        QMessageBox::information(this, "提示", "都说了要向前了~");
    } else if (targetPage > totalPage) {
        targetPage = totalPage;
        QMessageBox::information(this, "提示", QString("已经到底了~"));
    }

    //执行跳转
    currentPage = targetPage;
    searchTickets(currentPage);
    // 更新按钮状态
    ui->btn_prev->setEnabled(currentPage > 1);
    ui->btn_next->setEnabled(currentPage < totalPage);
}

void Deal::searchTickets(int pageNum)
{
    QElapsedTimer timer;
    timer.start(); // 计时，用于调试优化效果

    totalPage = getTotalPage();

    // 如果总页数为0（无数据），直接清空表格并返回
    if (totalPage == 0) {
        ui->tableWidget_tickets->setUpdatesEnabled(false);
        ui->tableWidget_tickets->setRowCount(0);
        ui->tableWidget_tickets->setUpdatesEnabled(true);
        ui->label_pageInfo->setText("0");
        ui->btn_prev->setEnabled(false);
        ui->btn_next->setEnabled(false);
        updatePageContainerText();
        return;
    }

    QString from = ui->lineEdit_from->text().trimmed();
    QString to = ui->lineEdit_to->text().trimmed();


    // 检查数据库连接
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid()) {
        QMessageBox::warning(this, "错误", "数据库连接未初始化！");
        return;
    }

    QList<int> favoriteTicketIds;
    if (!currentUserID.isEmpty()) {
        QSqlQuery favQuery;
        favQuery.prepare("SELECT TicketID FROM favorites WHERE UserID = :uid");
        favQuery.bindValue(":uid", currentUserID);
        if (favQuery.exec()) {
            while(favQuery.next()) {
                favoriteTicketIds.append(favQuery.value(0).toInt());
            }
        }
        favQuery.finish();
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            QMessageBox::warning(this, "错误", "数据库连接失败：" + db.lastError().text());
            return;
        }
    }

    QString startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString searchTime=ui->dateEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss");

    QString sql = "SELECT flight_id, flight_number, departure_city, arrival_city, departure_time, arrival_time, "
                  "price, departure_airport, arrival_airport, airline_company, availableSeat "
                  "FROM flight_info WHERE status = 'On Time' "
                  "AND departure_time >= :start_time "
                  "AND availableSeat > 0 ";

    // 动态添加条件（保持不变，但确保空格正确）
    if (!from.isEmpty()) {
        sql += "AND departure_city LIKE :from ";
    }
    if (!to.isEmpty()) {
        sql += "AND arrival_city LIKE :to ";
    }

    int offset = (pageNum - 1) * PAGE_SIZE;
    offset = qMax(offset, 0);
    sql += QString("ORDER BY departure_time ASC LIMIT %1, %2").arg(offset).arg(PAGE_SIZE);


    QSqlQuery query(db);
    query.setForwardOnly(true);  // 优化性能，避免结果集缓存问题
    query.setNumericalPrecisionPolicy(QSql::LowPrecisionInt32);

    if (!query.prepare(sql)) {
        QMessageBox::critical(this, "查询错误", "SQL 预处理失败：" + query.lastError().text());
        return;
    }
    if(searchTime>startTime)
        query.bindValue(":start_time", searchTime);
    else
        query.bindValue(":start_time", startTime);


    if (!from.isEmpty()) {
        query.bindValue(":from", from + "%");
    }
    if (!to.isEmpty()) {
        query.bindValue(":to", to + "%");
    }


    //执行查询（添加错误详情调试）
    if (!query.exec()) {
        QMessageBox::critical(this, "查询错误",
            "SQL执行失败：" + query.lastError().text() + "\n"
            "原生错误码：" + query.lastError().nativeErrorCode() + "\n"
            "预处理后的SQL：" + query.lastQuery());
        return;
    }

    ui->tableWidget_tickets->setUpdatesEnabled(false);
    ui->tableWidget_tickets->setRowCount(0);
    int row = 0;

    while (query.next()) {
        ui->tableWidget_tickets->insertRow(row);

        int ticketId = query.value("flight_id").toInt();


        // 设置表格数据（列索引与表头对应）
        ui->tableWidget_tickets->setItem(row, 0, new QTableWidgetItem(query.value(1).toString()));
        ui->tableWidget_tickets->setItem(row, 1, new QTableWidgetItem(query.value(2).toString() + "-" + query.value(7).toString()));
        ui->tableWidget_tickets->setItem(row, 2, new QTableWidgetItem(query.value(3).toString() + "-" + query.value(8).toString()));

        // 日期时间格式化（确保显示正确）
        QDateTime depTime = query.value(4).toDateTime();
        QDateTime arrTime = query.value(5).toDateTime();
        ui->tableWidget_tickets->setItem(row, 3, new QTableWidgetItem(depTime.toString("yyyy-MM-dd HH:mm")));
        ui->tableWidget_tickets->setItem(row, 4, new QTableWidgetItem(arrTime.toString("yyyy-MM-dd HH:mm")));

        // 价格保留2位小数
        ui->tableWidget_tickets->setItem(row, 5, new QTableWidgetItem(QString::number(query.value(6).toDouble(), 'f', 2)));

        // 可用座位数
        ui->tableWidget_tickets->setItem(row, 6, new QTableWidgetItem(query.value(10).toString()));
        // 公司名称
        ui->tableWidget_tickets->setItem(row, 7, new QTableWidgetItem(query.value(9).toString()));

        // 添加订票按钮
        QPushButton *btnBook = new QPushButton("订票");
        btnBook->setStyleSheet("background-color:#4CAF50; color:white; border:none; padding:2px 8px; border-radius:3px;");
        btnBook->setProperty("ticketId", ticketId);
        connect(btnBook, &QPushButton::clicked, this, &Deal::onBookTicket);
        ui->tableWidget_tickets->setCellWidget(row, 8, btnBook);

        bool isFavorited = favoriteTicketIds.contains(ticketId);
        QPushButton *btnFav = new QPushButton(isFavorited ? "已收藏" : "收藏");
        btnFav->setProperty("ticketId", ticketId);
        btnFav->setStyleSheet("border: 1px solid lightgray; padding:1px; border-radius:3px;");

        if (isFavorited) {
            btnFav->setEnabled(false);
        }
        connect(btnFav, &QPushButton::clicked, this, &Deal::onAddFavorite);
        ui->tableWidget_tickets->setCellWidget(row, 9, btnFav);

        row++;
    }

    //调整表格列宽，优化显示
    ui->tableWidget_tickets->setUpdatesEnabled(true);
    ui->tableWidget_tickets->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    updatePageContainerText();
    // 更新分页按钮状态（核心：禁用/启用上一页/下一页）
    ui->btn_prev->setEnabled(currentPage > 1);
    ui->btn_next->setEnabled(currentPage < totalPage);

    // 调试：输出查询耗时
    qDebug() << "查询耗时：" << timer.elapsed() << "ms，查询条数：" << row;
}

void Deal::on_btn_search_clicked()
{
    currentPage = 1;
    searchTickets(currentPage);
}


void Deal::onBookTicket()
{
    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }

    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int ticketId = btn->property("ticketId").toInt();
    
    int userId = currentUserID.toInt();

    // 打开订单对话框
    OrderDialog *dialog = new OrderDialog(ticketId, userId, this);
    if (dialog->exec() == QDialog::Accepted) {
        refreshTicketList();
    }
    delete dialog;
}

void Deal::refreshTicketList()
{
    searchTickets(currentPage);
}

void Deal::on_Single_Center_clicked()
{
    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }
    m_userProfilePage->getData(currentUserID);
    ui->stackedWidget->setCurrentWidget(m_userProfilePage);
}

void Deal::on_Deal_2_clicked()
{
    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }
    m_personalCenterPage->refreshOrderList();
    ui->stackedWidget->setCurrentWidget(m_personalCenterPage);

}
void Deal::showTicketSearchPage()
{
    currentPage = 1;
    searchTickets(currentPage);
    ui->stackedWidget->setCurrentWidget(ui->page_tickets);
}
void Deal::onAddFavorite()
{
    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int ticketId = btn->property("ticketId").toInt();

    QSqlQuery insertQuery;
    insertQuery.prepare("INSERT INTO favorites (UserID, TicketID) VALUES (?, ?)");
    insertQuery.addBindValue(currentUserID);
    insertQuery.addBindValue(ticketId);

    if (insertQuery.exec()) {
        QMessageBox::information(this, "成功", "已添加到收藏夹！");
        btn->setEnabled(false);
        btn->setText("已收藏");
    } else {
        if (insertQuery.lastError().text().contains("Duplicate") ||
            insertQuery.lastError().text().contains("UNIQUE")) {
            QMessageBox::information(this, "提示", "您已经收藏过该行程了！");
        } else {
            QMessageBox::critical(this, "错误", "收藏失败：" + insertQuery.lastError().text());
        }
    }
    insertQuery.finish();
}

void Deal::on_favorite_button_clicked()
{
    // 1. 检查是否登录
    if (currentUserID.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先登录！");
        return;
    }

    // 2. 刷新收藏列表数据 (确保显示最新数据)
    m_favoriteDialogPage->refreshFavoriteList();

    // 3. 切换界面显示到收藏夹页面
    ui->stackedWidget->setCurrentWidget(m_favoriteDialogPage);
}
void Deal::on_home_clicked(){
    this->showTicketSearchPage();
}

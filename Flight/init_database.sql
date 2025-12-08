-- 订票系统数据库初始化脚本
-- 数据库名称: Flight

-- 用户表（如果不存在）
CREATE TABLE IF NOT EXISTS users (
  UserID int NOT NULL AUTO_INCREMENT PRIMARY KEY,
  Username varchar(20) DEFAULT NULL,
  IDCard varchar(18) DEFAULT NULL,
  Deal int DEFAULT NULL,
  PWord varchar(20) DEFAULT NULL,
  jianjie varchar(50) DEFAULT NULL,
  avatar longblob
);
ALTER TABLE users ADD COLUMN Balance DECIMAL(10, 2) NOT NULL DEFAULT 0.00;

-- 票务表（航班/车次信息）
CREATE TABLE flight_info(
  flight_id int NOT NULL AUTO_INCREMENT,
  flight_number varchar(10) NOT NULL,
  departure_city varchar(20) NOT NULL,
  arrival_city varchar(20) NOT NULL,
  departure_time datetime NOT NULL,
  arrival_time datetime NOT NULL,
  price decimal(10,2) DEFAULT '700.00',
  departure_airport varchar(20) NOT NULL,
  arrival_airport varchar(20) NOT NULL,
  airline_company varchar(20) NOT NULL,
  checkin_start_time datetime NOT NULL,
  checkin_end_time datetime NOT NULL,
  status varchar(10) NOT NULL,
  availableSeat int DEFAULT 0,
  PRIMARY KEY (`flight_id`),
  UNIQUE KEY `flight_number` (`flight_number`)
)


-- 订单表
CREATE TABLE IF NOT EXISTS orders (
    OrderID INT AUTO_INCREMENT PRIMARY KEY,
    UserID INT NOT NULL COMMENT '用户ID',
    TicketID INT NOT NULL COMMENT '票务ID',
    OrderNo VARCHAR(50) NOT NULL UNIQUE COMMENT '订单号',
    PassengerName VARCHAR(50) NOT NULL COMMENT '乘客姓名',
    PassengerIDCard VARCHAR(18) COMMENT '乘客身份证号',
    ContactPhone VARCHAR(20) COMMENT '联系电话',
    TicketCount INT NOT NULL DEFAULT 1 COMMENT '购票数量',
    TotalPrice DECIMAL(10, 2) NOT NULL COMMENT '总价',
    OrderStatus VARCHAR(20) DEFAULT 'Pending' COMMENT '订单状态：Pending-待支付, Paid-已支付, Cancelled-已取消, Completed-已完成',
    OrderTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '下单时间',
    FOREIGN KEY (UserID) REFERENCES users(UserID) ON DELETE CASCADE,
    FOREIGN KEY (TicketID) REFERENCES flight_info(flight_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS favorites (
    FavoriteID INT AUTO_INCREMENT PRIMARY KEY,
    UserID INT NOT NULL,
    TicketID INT NOT NULL,
    CreatedTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (UserID) REFERENCES users(UserID) ON DELETE CASCADE,
    FOREIGN KEY (TicketID) REFERENCES flight_info(flight_id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_ticket (UserID, TicketID)
);

create table passengers
(
    PassengerID int auto_increment primary key,
    UserID      int not null,
    Name        varchar(50) not null,
    IDCard      varchar(18) not null,
    Phone       varchar(11) not null,
    CreatedTime datetime default CURRENT_TIMESTAMP null,
    constraint unique_user_idcard unique (UserID, IDCard),
    constraint passengers_ibfk_1 foreign key (UserID) references users (UserID)
);


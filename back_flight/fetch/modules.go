package main

import "time"

type Flight struct {
	FlightNumber     string    `json:"flight_number"`      // 航班机号
	DepartureCity    string    `json:"departure_city"`     // 出发地
	ArrivalCity      string    `json:"arrival_city"`       // 目的地
	DepartureTime    time.Time `json:"departure_time"`     // 出发时间
	ArrivalTime      time.Time `json:"arrival_time"`       // 到达时间
	Price            float64   `json:"price"`              // 价格
	DepartureAirport string    `json:"departure_airport"`  // 出发机场
	ArrivalAirport   string    `json:"arrival_airport"`    // 到达机场
	AirlineCompany   string    `json:"airline_company"`    // 航空公司
	CheckinStartTime time.Time `json:"checkin_start_time"` // 值机开始时间
	CheckinEndTime   time.Time `json:"checkin_end_time"`   // 值机结束时间
	Status           string    `json:"status"`             // 当前状态
	AvailableSeat    int       `json:"available_seat"`     // 可用座位数
}

type DatabaseConfig struct {
	Hostname string `json:"hostname"`
	Port     int    `json:"port"`
	Database string `json:"database"`
	Username string `json:"username"`
	Password string `json:"password"`
}

type FlightRequest struct {
	DepartureCityCode string `json:"departureCityCode"`
	ArriveCityCode    string `json:"arriveCityCode"`
	PageNo            int    `json:"pageNo"`
}

// 重点：扩展 FlightResponse 结构体，兼容可能的嵌套价格字段
type FlightResponse struct {
	TotalPage         int          `json:"totalPage"`
	DepartureCityName string       `json:"departureCityName"`
	ArriveCityName    string       `json:"arriveCityName"`
	ScheduleVOList    []ScheduleVO `json:"scheduleVOList"`
}

// 重点：扩展 ScheduleVO，兼容可能的嵌套价格字段和备用座位数字段
type ScheduleVO struct {
	FlightNo   string   `json:"flightNo"`
	DepartDate string   `json:"departDate"`
	DepartTime string   `json:"departTime"`
	ArriveTime string   `json:"arriveTime"`
	Price      float64  `json:"price"` // 直接价格字段
	PriceInfo  struct { // 备用：嵌套价格字段（部分接口可能返回）
		AdultPrice float64 `json:"adultPrice"`
	} `json:"priceInfo"`
	DepartPortName     string `json:"departPortName"`
	ArrivePortName     string `json:"arrivePortName"`
	AirlineCompanyName string `json:"airlineCompanyName"`
	AvailableSeat      int    `json:"availableSeat"` // 主座位数字段
	SeatRemaining      int    `json:"seatRemaining"` // 备用：座位剩余字段
}

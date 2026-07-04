#pragma once
#include <stdint.h>

// 总控界面APP ID
#define CONTROL_SERVER_APP_ID		0x1001 // 

// CPS数字孪生模块 DEV ID
#define CPS_DT_DEVICE_ID							401  // 1025


#pragma pack(push, 4)
////////////////////////////////通用消息定义////////////////////////////////////////
// 错误消息
#define MSG_RSP_INFO				0x500
typedef struct
{
	int		error_code;
	char	error_msg[128];
}ST_MsgRsp;

// 通用消息应答
typedef struct
{
	int req_no;
	ST_MsgRsp rsp;
}ST_Rsp;

// 初始化消息
#define MSG_CMD_INIT				0x501
typedef struct
{
	int req_no;
}ST_CMDInit;

// 初始化消息应答
#define MSG_CMD_INIT_RSP			0x502
typedef struct
{
	int req_no;
	ST_MsgRsp rsp;
}ST_CMDInitRsp;

// 停止消息
#define MSG_CMD_STOP				0x503
typedef struct
{
	int req_no;
}ST_CMDStop;

// 停止消息应答
#define MSG_CMD_STOP_RSP			0x504
typedef struct
{
	int req_no;
	ST_MsgRsp rsp;
}ST_CMDStopRsp;

// Reset请求和应答
#define MSG_CMD_RESET				0x505
#define MSG_CMD_RESET_RSP				0x506
typedef struct
{
	int req_no;
}ST_CMDReset;

// Continue请求和应答
#define MSG_CMD_CONTINUE				0x507
#define MSG_CMD_CONTINUE_Rsp				0x508
typedef struct
{
	int req_no;
}ST_CMDContinue;

// 初始化消息
#define MSG_POSE				0x509
typedef struct
{
	double pose[6];
}ST_POSE;

// 目标位姿消息。数据结构与 MSG_POSE 相同：
// pose = [X, Y, Z, Rx, Ry, Rz]
#define MSG_TARGET_POSE			0x50A

#pragma pack(pop)


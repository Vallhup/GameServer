#pragma once

constexpr short PORT_NUM{ 9000 };

enum PacketType : char {
	CS_MOVE,
	CS_ATTACK,
	CS_ATTACK_END,

	SC_MOVE_OBJECT,
	SC_ADD,
	SC_REMOVE,
	SC_ATTACK,
	SC_ATTACK_END,
	SC_DEAD,
	SC_REVIVE,
	SC_STAT_UPDATE
};

enum MoveDirection : char {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	UPLEFT,
	UPRIGHT,
	DOWNLEFT,
	DOWNRIGHT
};

#pragma pack(push, 1)

struct CS_MOVE_PACKET {
	unsigned char size;
	char type;
	float angle;
	char direction;
	bool isRun;
};

struct CS_ATTACK_PACKET {
	unsigned char size;
	char type;
	float x;
	float y;
	float z;
};

struct CS_ATTACK_END_PACKET {
	unsigned char size;
	char type;
};

struct SC_MOVE_PACKET {
	unsigned char size;
	char type;
	int id;
	float angle;
	float x;
	float y;
	float z;
	bool isMove;
	bool isRun;
};

struct SC_ADD_PACKET {
	unsigned char size;
	char type;
	int id;
	int ownerId;
	float x;
	float y;
	float z;
};

struct SC_REMOVE_PACKET {
	unsigned char size;
	char type;
	int id;
};

struct SC_ATTACK_PACKET {
	unsigned char size;
	char type;
	int id;
};

struct SC_ATTACK_END_PACKET {
	unsigned char size;
	char type;
	int id;
};

struct SC_DEAD_PACKET {
	unsigned char size;
	char type;
	int id;
};

struct SC_REVIVE_PACKET {
	unsigned char size;
	char type;
	int id;
	float x;
	float y;
	float z;
};

struct SC_STAT_UPDATE_PACKET {
	unsigned char size;
	char type;
	int id;
};

#pragma pack(pop)
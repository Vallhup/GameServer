#pragma once

constexpr short PORT_NUM{ 9000 };

enum PacketType : char {
	CS_MOVE,
	CS_ATTACK,

	SC_MOVE_OBJECT,
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
	char direction;
	bool isRun;
};

struct CS_ATTACK_PACKET {
	unsigned char size;
	char type;
};

struct SC_MOVE_PACKET {
	unsigned char size;
	char type;
	int id;	
	float x;
	float y;
};

struct SC_ATTACK_PACKET {
	unsigned char size;
	char type;
	int id;
};

struct SC_STAT_UPDATE_PACKET {
	unsigned char size;
	char type;
	int hp;
};

#pragma pack(pop)
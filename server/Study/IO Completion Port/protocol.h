constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

constexpr int MAX_USER = 10000;
constexpr int MAX_NPC = 200000;

constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;

enum PacketID : char {
	CS_LOGIN,
	CS_MOVE,
	CS_CHAT,
	CS_ATTAK,
	CS_TELEPORT,
	CS_LOGOUT,

	SC_LOGIN_INFO,
	SC_ADD_OBJECT,
	SC_REMOVE_OBJECT,
	SC_MOVE_OBJECT,
	SC_CHAT,
	SC_LOGIN_OK,
	SC_LOGIN_FAIL,
	SC_STAT_CHANGE
};

enum MoveDirection : char {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned char size;
	char	type;
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned char size;
	char	type;
	char	direction;
	unsigned int move_time;
};

struct CS_CHAT_PACKET {
	unsigned char size;
	char type;
	char message[CHAT_SIZE];
};

struct CS_TELEPORT_PACKET {
	unsigned char size;
	char	type;
};

struct CS_LOGOUT_PACKET {
	unsigned char size;
	char	type;
};

struct SC_LOGIN_INFO_PACKET {
	unsigned char size;
	char	type;
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	short	x, y;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int		id;
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int		id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int		id;
	short	x, y;
	unsigned int move_time;
};

struct SC_CHAT_PACKET {
	unsigned char size;
	char type;
	short id;
	char message[CHAT_SIZE];
};

struct SC_LOGIN_OK_PACKET {
	unsigned char size;
	char	type;
};

struct SC_LOGIN_FAIL_PACKET {
	unsigned char size;
	char	type;
};

struct SC_STAT_CHANGEL_PACKET {
	unsigned char size;
	char	type;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
};

#pragma pack (pop)
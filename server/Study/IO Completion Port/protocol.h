constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

constexpr int MAX_USER = 20000;
constexpr int MAX_NPC = 200000;

constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;

enum PacketID : char {
	CS_LOGIN,
	CS_MOVE,
	CS_CHAT,
	CS_ATTACK,
	CS_TELEPORT,
	CS_LOGOUT,

	CS_PARTY_REQUEST,
	CS_PARTY_RESPONSE,
	CS_PARTY_LEAVE,

	CS_USE_ITEM,

	SC_LOGIN_INFO,
	SC_ADD_OBJECT,
	SC_REMOVE_OBJECT,
	SC_MOVE_OBJECT,
	SC_CHAT,
	SC_LOGIN_OK,
	SC_LOGIN_FAIL,
	SC_STAT_CHANGE,
	SC_ATTACK_NOTIFY,

	SC_PARTY_REQUEST,
	SC_PARTY_RESULT,
	SC_PARTY_UPDATE,
	SC_PARTY_EXP_SHARE,
	SC_PARTY_DISBAND,

	SC_ADD_ITEM,
	SC_USE_ITEM_OK,
	SC_USE_ITEM_FAIL
};

enum MoveDirection : char {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

enum PartyMemberState : char {
	ONLINE_ALIVE,
	ONLINE_DEAD,
	OFFLINE
};

enum ItemId : char {
	HpPotion
};

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned char size;
	char	type;
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned char size;
	char		  type;
	char		  direction;
	unsigned int  move_time;
};

struct CS_CHAT_PACKET {
	unsigned char size;
	char type;
	char message[CHAT_SIZE];
};

struct CS_ATTACK_PACKET {
	unsigned char size;
	char type;
	unsigned int  attack_time;
};

struct CS_TELEPORT_PACKET {
	unsigned char size;
	char	type;
};

struct CS_LOGOUT_PACKET {
	unsigned char size;
	char	type;
};

struct CS_PARTY_REQUEST_PACKET {
	unsigned char size;
	char type;
	int targetId;
};

struct CS_PARTY_RESPONSE_PACKET {
	unsigned char size;
	char type;
	bool acceptFlag;;
};

struct CS_PARTY_LEAVE_PACKET {
	unsigned char size;
	char type;
};

struct CS_USE_ITEM_PACKET {
	unsigned char size;
	char type;
	char itemId;
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
	int id;
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

struct SC_STAT_CHANGE_PACKET {
	unsigned char size;
	char	type;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
};

struct SC_ATTACK_NOTIFY_PACKET {
	unsigned char size;
	char type;
	int attackerId;
	short x[4], y[4];
};

struct SC_PARTY_REQUEST_PACKET {
	unsigned char size;
	char type;
	int fromId;
};

struct SC_PARTY_RESULT_PACKET {
	unsigned char size;
	char type;
	int targetId;
	bool acceptFlag;
};

struct MemberInfo {
	int id;
	char state;
};

struct SC_PARTY_UPDATE_PACKET {
	unsigned char size;
	char type;
	int partyId;
	short memberCount;
	MemberInfo members[];
};

struct SC_PARTY_EXP_SHARE_PACKET {
	unsigned char size;
	char type;
	int memberId;
	int exp;
};

struct SC_PARTY_DISBAND_PACKET {
	unsigned char size;
	char type;
	int partyId;
};

struct SC_USE_ITEM_OK_PACKET {
	unsigned char size;
	char type;
	char itemId;
};

struct SC_ADD_ITEM_PACKET {
	unsigned char size;
	char type;
	char itemId;
	int count;
};

struct SC_USE_ITEM_FAIL_PACKET {
	unsigned char size;
	char type;
	char itemId;
};

#pragma pack (pop)
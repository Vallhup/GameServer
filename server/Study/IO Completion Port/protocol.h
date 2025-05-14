constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

constexpr int MAX_USER = 5000;

constexpr int W_WIDTH = 400;
constexpr int W_HEIGHT = 400;

enum PacketID : char {
	CS_LOGIN,
	CS_MOVE,
	CS_CHAT,

	SC_LOGIN_INFO,
	SC_ADD_PLAYER,
	SC_REMOVE_PLAYER,
	SC_MOVE_PLAYER,
	SC_CHAT_BROADCAST
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
	char message[BUF_SIZE];
};

struct SC_LOGIN_INFO_PACKET {
	unsigned char size;
	char	type;
	short	id;
	short	x, y;
};

struct SC_ADD_PLAYER_PACKET {
	unsigned char size;
	char	type;
	short	id;
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_PLAYER_PACKET {
	unsigned char size;
	char	type;
	short	id;
};

struct SC_MOVE_PLAYER_PACKET {
	unsigned char size;
	char	type;
	short	id;
	short	x, y;
	unsigned int move_time;
};

struct SC_CHAT_BROADCAST_PACKET {
	unsigned char size;
	char type;
	short id;
	char message[BUF_SIZE];
};

#pragma pack (pop)


template<typename Packet>
std::vector<char> Serialize(Packet& packet)
{
	std::vector<char> out(sizeof(Packet));
	std::memcpy(out.data(), &packet, sizeof(Packet));

	return out;
}

template<typename Packet>
Packet Deserialize(const std::vector<char>& buf)
{
	Packet packet{};
	size_t copySize = std::min<size_t>(buf.size(), sizeof(Packet));
	std::memcpy(&packet, buf.data(), copySize);

	return packet;
}
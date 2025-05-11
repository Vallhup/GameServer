#include "pch.h"
#include "Session.h"

Session::~Session()
{ 
	std::cout << "Session" << std::endl;

	ResponseLeavePacket responPacket(_id);

	if(auto locked = _service.lock()) {
		for (auto& [id, session] : locked->_sessions) {
			SessionPtr p = session.load();
			if(nullptr != p)
				p->Send(responPacket.Serialize());
		}
	}

	if (_recvOver._owner != nullptr) {
		_recvOver._owner = nullptr;
	}

	if (_sendOver._owner != nullptr) {
		_sendOver._owner = nullptr;
	}

	closesocket(_socket);
}

void Session::Send(const std::vector<char>& data)
{
	_sendQueue.Push(data);

	if (not _isSending.exchange(true)) {
		doSend();
	}
}

void Session::doRecv()
{
	DWORD recvFlag = 0;

	_recvOver.Init();

	if(_recvOver._owner == nullptr)
		_recvOver._owner = shared_from_this();
	
	_recvOver._wsaBuf[0].buf = _recvOver._buffer.GetWritePos();
	_recvOver._wsaBuf[0].len = _recvOver._buffer.GetFreeSize();

	int result = WSARecv(_socket, _recvOver._wsaBuf, 1, NULL, &recvFlag, reinterpret_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			//errorDisplay("Recv : ", error);
			std::cout << "Recv Error\n";
		}
	}
}

void Session::doSend()
{
	std::shared_ptr<std::vector<char>> sendData;
	
	if (not _sendQueue.tryPop(sendData)) {
		_isSending.store(false);
		return;
	}

	_sendOver.Init();

	if (_sendOver._owner == nullptr)
		_sendOver._owner = shared_from_this();

	// scatter-gather 적용 X
	_sendOver.SetBuffer(sendData);

	DWORD bytesSent{ 0 };
	if (SOCKET_ERROR == WSASend(_socket, &_sendOver._wsaBuf[0], 1, &bytesSent, 0, reinterpret_cast<LPWSAOVERLAPPED>(&_sendOver), NULL)) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			std::cout << "Session Send Error\n";
			_isSending.store(false);
		}
	}
}

void Session::RecvCallback(DWORD numBytes)
{
	_recvOver._buffer.Write(nullptr, numBytes);

	std::vector<char> readBuffer(numBytes);
	_recvOver._buffer.Read(readBuffer.data(), numBytes);

	ProcessPacket(readBuffer);
	
	doRecv();
}

void Session::SendCallback()
{
	std::cout << "SendCallback\n";
	doSend();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(ExpOver* expOver, int numOfBytes)
{
	std::cout << "Session Dispatch" << std::endl;
	switch (expOver->_operationType) {
	case OperationType::Recv:
		RecvCallback(numOfBytes);
		break;

	case OperationType::Send:
		SendCallback();
		break;

	default:
		break;
	}
}

GameSession::~GameSession()
{
	std::cout << "GameSession Delete\n";

	// TODO : 추후 자원 해제가 필요하게 되면 추가
}

bool GameSession::ProcessPacket(const std::vector<char>& packet)
{
	char packetType = packet[1];

	switch (packetType)
	{
	case RESPONSE_CONNECT: {
		ResponseConnectPacket responsePacket(_id);
		Send(responsePacket.Serialize());

		ResponseEnterPacket responseEnterPacket(_id, _pos);

		if (auto locked = _service.lock()) {
			for (auto& [id, session] : locked->_sessions) {
				SessionPtr p = session.load();
				if ((nullptr != p) and (id != _id))
					p->Send(responseEnterPacket.Serialize());
			}
		}

		if (auto locked = _service.lock()) {
			for (auto& [id, session] : locked->_sessions) {
				SessionPtr p = session.load();
				if ((nullptr != p) and (id != _id)) {
					ResponseEnterPacket responseEnterPacket(id, static_pointer_cast<GameSession>(p)->GetPos());
					Send(responseEnterPacket.Serialize());
				}
			}
		}

		break;
	}

	case RESPONSE_MOVE: {
		char packetDirection = packet[6];

		switch (packetDirection) {
		case MOVE_UP:    _pos._yPos = std::max<short>(_pos._yPos - 1, 0); break;
		case MOVE_DOWN:  _pos._yPos = std::min<short>(_pos._yPos + 1, 7); break;
		case MOVE_LEFT:  _pos._xPos = std::max<short>(_pos._xPos - 1, 0); break;
		case MOVE_RIGHT: _pos._xPos = std::min<short>(_pos._xPos + 1, 7); break;
		}

		ResponseMovePacket responsePacket(_id, _pos);

		if (auto locked = _service.lock()) {
			for (auto& [id, session] : locked->_sessions) {
				SessionPtr p = session.load();
				if(nullptr != p)
					p->Send(responsePacket.Serialize());
			}
		}

		break;
	}

	default:
		std::cout << "Error Invalid Packet Type\n";
		return false;
	}

	return true;
}

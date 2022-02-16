NetLib를 사용 하여 만드는 에코서버 예제
=======================================

> 상속받아 클래스 구현


    class EchoServer : public NetLib::LanServer {

    public:

      bool OnConnectionRequest(WCHAR* szIP, short port);
      void OnClientJoin(NetLib::SESSIONID sessionID);
      void OnClientLeave(NetLib::SESSIONID sessionID);

      void OnRecv(NetLib::SESSIONID sessionID, NetLib::PacketBuffer* packet);
      void OnSend(NetLib::SESSIONID sessionID, DWORD SendSize);

      void OnWorkerThreadBegin();
      void OnWorkerThreadEnd();

      void OnError(int ErrorCode, WCHAR* ErrorMessage);

      int sendSize;
      int recvSize;
      int diff;
    };
    
    bool EchoServer::OnConnectionRequest(WCHAR * szIP, short port)
    {
      wprintf(L"%s : %d connectRequest\n", szIP, port);
      return true;
    }

    void EchoServer::OnClientJoin(NetLib::SESSIONID sessionID)
    {
      wprintf(L"%I64d client join\n", sessionID);

      __int64 login = 0x7fffffffffffffff;

      NetLib::PacketBuffer* loginPacket = NetLib::PacketBuffer::Alloc();
      loginPacket->AllocPos = 2;

      *loginPacket << login;


      SendPacket(sessionID, loginPacket);

      loginPacket->FreePos = 3;
      NetLib::PacketBuffer::Free(loginPacket);
    }

    void EchoServer::OnClientLeave(NetLib::SESSIONID sessionID)
    {
      wprintf(L"%I64d client leave\n", sessionID);
    }

    void EchoServer::OnRecv(NetLib::SESSIONID sessionID, NetLib::PacketBuffer * packet)
    {
      recvSize += packet->GetDataSize();

      NetLib::PacketBuffer *pac = NetLib::PacketBuffer::Alloc();
      pac->AllocPos = 3;

      __int64 data;
      *packet >> data;
      *pac << data;

      if(!SendPacket(sessionID, pac))
      {
        OnError(-1, const_cast<WCHAR *>(L"SendPacket Return False"));
      }

      pac->FreePos = 4;
      NetLib::PacketBuffer::Free(pac);
    }

    void EchoServer::OnSend(NetLib::SESSIONID sessionID, DWORD sendSize)
    {
      if (sendSize != 8)
        NetLib::CrashDump::Crash();

      this->sendSize += sendSize;
      //wprintf(L"%I64d session send %d size\n", sessionID, sendSize);
    }

    void EchoServer::OnWorkerThreadBegin()
    {
    }

    void EchoServer::OnWorkerThreadEnd()
    {
    }

    void EchoServer::OnError(int ErrorCode, WCHAR * ErrorMessage)
    {
      wprintf(L"%d: %s\n", ErrorCode, ErrorMessage);
      NetLib::CrashDump::Crash();
    }
    
    
    
> 메인함수 구현
    
    int main()
    {
      NetLib::Log::GetInstance()->LogDir(const_cast<WCHAR *>(L"Alloc"));
      NetLib::Log::GetInstance()->SetConsole(false);
      EchoServer echo;

      bool result = echo.Start(const_cast<WCHAR *>(L"127.0.0.1"), 6000, 6, 6, false, 100);

      if (!result)
        return 0;

      echo.diff = 0;
      echo.recvSize = 0;
      echo.sendSize = 0;


      while (1) {
        Sleep(1000);
        echo.diff += echo.recvSize - echo.sendSize;
        wprintf(L"%d recv %d send not send Size : %d\n", echo.recvSize, echo.sendSize, echo.diff);
        wprintf(L"ALLOC : %d USE : %d\n",NetLib::PacketBuffer::m_freeList.GetAllocCount(), NetLib::PacketBuffer::m_freeList.GetUseCount());
        echo.recvSize = 0;
        echo.sendSize = 0;
      };

      return 0;
    }

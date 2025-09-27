#ifndef MINBIT_CORE_H
#define MINBIT_CORE_H

#include <vector>
#include <mutex>
#include <chrono>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <queue>

#include "IStream.h"

class MinBiTCore {
    public:
        enum class WriteMode {
            IMMEDIATE,
            BULK
        };

        struct PacketLengthEntry {
            uint8_t header;
            int length;
        };

        class Request {
        public:
            enum class Status {
                WAITING,
                CHARACTERIZED,
                COMPLETE,
                TIMEDOUT
            };

            enum class Type {
                INCOMING,
                OUTGOING
            };

            Request(uint8_t header, Type type);

            void Start();
            void SetStatus(Status newStatus);
            void SetResponseHeader(uint8_t responseHeader);
            void SetExpectedLength(int16_t expectedLength);
            void SetPayloadLength(std::size_t payloadLength);
            void SetTotalPacketLength(std::size_t totalPacketLength);

            Status GetStatus();
            int64_t GetId() const;
            uint8_t GetHeader() const;
            uint8_t GetResponseHeader();
            int16_t GetExpectedLength();
            std::size_t GetPayloadLength();
            std::size_t GetTotalPacketLength();
            bool IsIncoming();
            bool IsOutgoing();
            bool IsWaiting();
            bool IsCharacterized();
            bool IsComplete();
            bool IsTimedOut();

            std::chrono::steady_clock::time_point GetSentTime();

            Status WaitSync(int pollIntervalMs = 5);

        private:
            static std::atomic<int64_t> nextId;
            int64_t id;
            uint8_t header;
            uint8_t responseHeader;
            int16_t expectedLength;
            std::size_t payloadLength;
            std::size_t totalPacketLength;
            Status status;
            Type type;
            std::chrono::steady_clock::time_point sentTime;
            std::mutex requestMutex;
        };

        using ReadHandler = std::function<void(std::shared_ptr<MinBiTCore::Request>)>;

        MinBiTCore(std::string name, std::shared_ptr<IStream> stream);
        ~MinBiTCore();

        //Sets read handler
        void setReadHandler(ReadHandler handler);

        // Gets stream object
        std::shared_ptr<IStream> getStream();

        // Set writing mode
        void setWriteMode(WriteMode mode);

        // Set request timeout
        void setRequestTimeout(uint16_t timeoutMs);

        // Loads packet length information
        void loadOutgoingByRequest(std::unordered_map<uint8_t, int16_t>* map);
        void loadOutgoingByResponse(std::unordered_map<uint8_t, int16_t>* map);
        void loadIncomingByRequest(std::unordered_map<uint8_t, int16_t>* map);

        // Writing functions
        std::shared_ptr<MinBiTCore::Request> writeRequest(uint8_t header);
        void writeBytes(const uint8_t* buffer, std::size_t length);
        void writeByte(uint8_t value);
        void writeFloat(float value);
        // Writes a 16 bit integer
        void writeInt16(int16_t data);
        // Writes packet
        void sendAll();

        // Reading functions
        void fetchData();
        uint8_t readByte();
        uint8_t peekByte();
        void readBytes(uint8_t* buffer, std::size_t len);
        float readFloat();
        int16_t readInt16();

        template <typename T>
        T readData();

        // Packet management

        // Processing loop
        std::shared_ptr<Request> getCurrentRequest();
        bool characterizePacket();
        // Gets the expected length for a header, returns false if not found
        bool getExpectedPacketLength(std::shared_ptr<Request> request, int16_t& length) const;
        bool getPacketParameters(int16_t expectedLength, std::size_t& payloadLength, std::size_t& totalPacketLength);
        bool getOutgoingRequest(std::shared_ptr<Request>& request);
        void checkForTimeouts();

        // Flushes the read buffer
        void flush();
        void flushRequest();
        bool clearRequest();
        std::size_t getReadBufferSize();
        std::size_t getWriteBufferSize();
        std::size_t getNumOutgoingRequests();
        std::size_t getReservedBytes();

    private:
        std::string name;
        std::shared_ptr<IStream> stream;
        std::vector<uint8_t> readBuffer;
        std::vector<uint8_t> writeBuffer;
        std::queue<std::shared_ptr<MinBiTCore::Request>> unsentRequests;
        std::queue<std::shared_ptr<MinBiTCore::Request>> outgoingRequests;
        // Current request being processed
        std::shared_ptr<Request> currRequest;
        // Current bytes reserved for reading
        std::size_t reservedBytes = 0;

        uint16_t requestTimeoutMs = 1000; // or make this configurable
        std::mutex dataMutex;

        // Outgoing packet lengths by request header
        std::unordered_map<uint8_t, int16_t>* outgoingByRequest;
        // Outgoing packet legnths by response header
        std::unordered_map<uint8_t, int16_t>* outgoingByResponse;
        // Incoming packet lengths by request header
        std::unordered_map<uint8_t, int16_t>* incomingByRequest;

        WriteMode writeMode = WriteMode::IMMEDIATE;

        //Read handler
		ReadHandler readHandler;

        // Buffer management
        void appendToReadBuffer(const uint8_t* data, std::size_t length);
        void appendToWriteBuffer(const uint8_t* data, std::size_t length);
};

template <typename T>
T MinBiTCore::readData() {
    T data;
    uint8_t buffer[sizeof(data)];
    MinBiTCore::readBytes(buffer, sizeof(data));

    std::memcpy(&data, buffer, sizeof(data));

    return data;
}

#endif // MINBIT_CORE_H
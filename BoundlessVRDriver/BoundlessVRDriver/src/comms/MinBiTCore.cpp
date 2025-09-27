#include "MinBiTCore.h"


using namespace boost;
using json = nlohmann::json;

std::atomic<int64_t> MinBiTCore::Request::nextId{ 1 };

MinBiTCore::Request::Request(uint8_t header, Request::Type type)
    : header(header),
    responseHeader(0),
    expectedLength(-1),
    payloadLength(0),
    totalPacketLength(0),
    status(Request::Status::WAITING),
    type(type),
    id(nextId.fetch_add(1))
{
}

void MinBiTCore::Request::Start() {
    std::lock_guard<std::mutex> lock(requestMutex);
    sentTime = std::chrono::steady_clock::now();
}

void MinBiTCore::Request::SetStatus(Status newStatus) {
    std::lock_guard<std::mutex> lock(requestMutex);
    status = newStatus;
}

void MinBiTCore::Request::SetResponseHeader(uint8_t responseHeader) {
    std::lock_guard<std::mutex> lock(requestMutex);
    this->responseHeader = responseHeader;
}

void MinBiTCore::Request::SetExpectedLength(int16_t expectedLength) {
    std::lock_guard<std::mutex> lock(requestMutex);
    this->expectedLength = expectedLength;
}

void MinBiTCore::Request::SetPayloadLength(std::size_t payloadLength) {
    std::lock_guard<std::mutex> lock(requestMutex);
    this->payloadLength = payloadLength;
}

void MinBiTCore::Request::SetTotalPacketLength(std::size_t totalPacketLength) {
    std::lock_guard<std::mutex> lock(requestMutex);
    this->totalPacketLength = totalPacketLength;
}

MinBiTCore::Request::Status MinBiTCore::Request::GetStatus() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return status;
}

int64_t MinBiTCore::Request::GetId() const {
    return id;
}

uint8_t MinBiTCore::Request::GetHeader() const {
    return header;
}

uint8_t MinBiTCore::Request::GetResponseHeader() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return responseHeader;
}

int16_t MinBiTCore::Request::GetExpectedLength() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return expectedLength;
}

std::size_t MinBiTCore::Request::GetPayloadLength() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return payloadLength;
}

std::size_t MinBiTCore::Request::GetTotalPacketLength() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return totalPacketLength;
}

bool MinBiTCore::Request::IsIncoming() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return type == Type::INCOMING;
}

bool MinBiTCore::Request::IsOutgoing() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return type == Type::OUTGOING;
}

bool MinBiTCore::Request::IsComplete() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return status == Status::COMPLETE;
}

bool MinBiTCore::Request::IsWaiting() {
    return status == Status::WAITING || status == Status::CHARACTERIZED;
}

bool MinBiTCore::Request::IsCharacterized() {
    return status == Status::CHARACTERIZED;
}

bool MinBiTCore::Request::IsTimedOut() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return status == Status::TIMEDOUT;
}

std::chrono::steady_clock::time_point MinBiTCore::Request::GetSentTime() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return sentTime;
}

std::future<MinBiTCore::Request::Status> MinBiTCore::Request::WaitAsync(int pollIntervalMs) {
    return std::async(std::launch::async, [this, pollIntervalMs]() {
        while (true) {
            {
                if (!IsWaiting())
                    return GetStatus();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
        }
    });
}

// DataProtocol handles serialization and communication of data packets over an IStream.
MinBiTCore::MinBiTCore(std::string name, std::shared_ptr<IStream> stream)
    : name(name), stream(std::move(stream)), currRequest(nullptr) {}

MinBiTCore::~MinBiTCore() {
    // Ensure the stream is closed on destruction.
    if (stream && stream->isOpen()) {
        stream->close();
    }
    // No dynamic allocations to clean up, but destructor ensures stream is closed.
}

void MinBiTCore::setReadHandler(ReadHandler handler) {
    // Set the callback to be invoked when data is read.
    readHandler = handler;
}

std::shared_ptr<IStream> MinBiTCore::getStream() {
    // Return the underlying stream.
    return stream;
}

void MinBiTCore::setEndianness(Endianness endianness) {
    // Set the byte order for serialization.
    this->endianness = endianness;
}

void MinBiTCore::setWriteMode(WriteMode mode) {
    // Set the mode for writing packets (immediate or buffered).
    this->writeMode = mode;
}

void MinBiTCore::setRequestTimeout(uint16_t timeoutMs) {
    this->requestTimeoutMs = timeoutMs;
}

bool MinBiTCore::loadPacketLengthsFromJson(const std::string& filePath) {
    try {
        std::ifstream ifs(filePath);
        auto j = json::parse(ifs);
        if (j.contains("outgoingByRequest") && j["outgoingByRequest"].is_array()) {
            outgoingByRequest.clear();
            for (const auto& entry : j["outgoingByRequest"]) {
                if (entry.contains("header") && entry.contains("length")) {
                    uint8_t header = entry["header"];
                    int length = entry["length"];
                    outgoingByRequest[header] = length;
                }
            }
        }
        if (j.contains("outgoingByResponse") && j["outgoingByResponse"].is_array()) {
            outgoingByResponse.clear();
            for (const auto& entry : j["outgoingByResponse"]) {
                if (entry.contains("header") && entry.contains("length")) {
                    uint8_t header = entry["header"];
                    int length = entry["length"];
                    outgoingByResponse[header] = length;
                }
            }
        }
        if (j.contains("incomingByRequest") && j["incomingByRequest"].is_array()) {
            incomingByRequest.clear();
            for (const auto& entry : j["incomingByRequest"]) {
                if (entry.contains("header") && entry.contains("length")) {
                    uint8_t header = entry["header"];
                    int length = entry["length"];
                    incomingByRequest[header] = length;
                }
            }
        }
        return true;
    }
    catch (std::exception e) {
        std::cerr << "File not found" << std::endl;
        return false;
    }
}

bool MinBiTCore::getExpectedPacketLength(std::shared_ptr<Request> request, int16_t& length) const {
    // Checks whether request is incoming or outgoing
    if (request->IsOutgoing()) {
        // Lengths by response header are the priority
        auto it = outgoingByResponse.find(request->GetResponseHeader());
        if (it != outgoingByResponse.end()) {
            length = it->second;
            return true;
        }

        // Otherwise searches by request header
        it = outgoingByRequest.find(request->GetHeader());
        if (it != outgoingByRequest.end()) {
            length = it->second;
            return true;
        }
    }
    else if (request->IsIncoming()) {
        // Searches incoming requests by request header
        auto it = incomingByRequest.find(request->GetHeader());
        if (it != incomingByRequest.end()) {
            length = it->second;
            return true;
        }
    }
    
    return false;
}

bool MinBiTCore::getPacketParameters(int16_t expectedLength, std::size_t& payloadLength, std::size_t& totalPacketLength) {
    payloadLength = 0;
    totalPacketLength = 1; // 1 byte for response header

    if (expectedLength == -1) {
        // Need at least 2 bytes: header + length byte
        if (getReadBufferSize() < 2) {
            return false;
        }
        uint8_t lengthByte;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            lengthByte = readBuffer[1];
        }
        // Sets expected payload length
        payloadLength = lengthByte;
        // Adds to total packet length
        totalPacketLength += 1 + payloadLength; // header + length byte + payload
    }
    else {
        // Updates expected payload length
        payloadLength = expectedLength;
        totalPacketLength += expectedLength;
    }
    return true;
}

std::shared_ptr<MinBiTCore::Request> MinBiTCore::getCurrentRequest() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return currRequest;
}

std::shared_ptr<MinBiTCore::Request> MinBiTCore::writeRequest(uint8_t header) {
    // Creates new outgoing request
    auto request = std::make_shared<Request>(header, Request::Type::OUTGOING);
    {
        // Adds to unsent requests
        std::lock_guard<std::mutex> lock(dataMutex);
        unsentRequests.push(request);
    }
    writeByte(header);
    return request;
}

void MinBiTCore::writeBytes(const uint8_t* buffer, std::size_t length) {
    // Appends data to write buffer
    appendToWriteBuffer(buffer, length);

    // Writes packet immediately if in immediate mode
    if (writeMode == WriteMode::IMMEDIATE) {
        sendAll();
    }
}

void MinBiTCore::writeByte(uint8_t value) {
    // Write a single byte.
    writeBytes(&value, sizeof(value));
}

void MinBiTCore::writeFloat(float value) {
    // Serialize a float with correct endianness and write.
    uint32_t networkValue = *reinterpret_cast<uint32_t*>(&value);
    if (endianness == Endianness::BigEndian) {
        networkValue = boost::endian::native_to_big(networkValue);
    } else {
        networkValue = boost::endian::native_to_little(networkValue);
    }
    writeBytes(reinterpret_cast<uint8_t*>(&networkValue), sizeof(networkValue));
}

void MinBiTCore::writeInt16(int16_t data) {
    // Serialize and write a 16-bit integer.
    uint8_t buffer[sizeof(data)];
    memcpy(buffer, &data, sizeof(data));
    writeBytes(buffer, sizeof(data));
}

void MinBiTCore::writeVector3f(const Eigen::Vector3f& vector) {
    // Write a 3D vector as three floats.
    for (int i = 0; i < 3; ++i) {
        writeFloat(static_cast<float>(vector(i)));
    }
}

void MinBiTCore::writeQuaternionf(const Eigen::Quaternionf& quaternion) {
    // Write a quaternion as four floats (coefficients order).
    for (int i = 0; i < 4; ++i) {
        writeFloat(static_cast<float>(quaternion.coeffs()(i)));
    }
}

void MinBiTCore::sendAll() {
    // Write the contents of the write buffer as a packet.
    std::lock_guard<std::mutex> lock(dataMutex);
    if (!stream || !stream->isOpen()) return;

    // Starts unsent requests (if there are any)
    for (int i = 0; i < unsentRequests.size(); i++)
    {
        // Dequeues from unsent requests
        std::shared_ptr<Request> request = unsentRequests.front();
        unsentRequests.pop();
        // Starts request
        request->Start();
        // Adds to outgoing request queue
        outgoingRequests.push(request);
    }

    // Writes data to stream
    size_t trueBufferSize = writeBuffer.size();
    stream->asyncWrite(writeBuffer.data(), trueBufferSize,
        [this, trueBufferSize](const boost::system::error_code& error, std::size_t bytesTransferred) {
            if (error) {
                std::cerr << "(" + name + ") Error writing bytes: " << error.message() << std::endl;
            }
            else if (bytesTransferred < trueBufferSize) {
                std::cerr << "(" + name + ") Partial write detected. Ensure all bytes are written." << std::endl;
            }
        });

    // Clears write buffer
    writeBuffer.clear();
}

void MinBiTCore::checkForTimeouts() {
    std::shared_ptr<Request> req;
    if (getOutgoingRequest(req)) {
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - req->GetSentTime()).count() > requestTimeoutMs) {
            std::cerr << "(" + name + ") Outgoing request with header " << int(req->GetHeader()) << " timed out after " << requestTimeoutMs << " ms." << std::endl;
            req->SetStatus(Request::Status::TIMEDOUT);
            if (currRequest != nullptr && currRequest->IsOutgoing()) {
                clearRequest();
            }
            else {
                std::lock_guard<std::mutex> lock(dataMutex);
                outgoingRequests.pop();
            }
            flush();
            // Calls read handler
            if (readHandler) {
                readHandler(req);
            }
        }
    }
}

bool MinBiTCore::characterizePacket() {
    // If bytes in buffer are still reserved, wait
    if (reservedBytes > 0) {
        return false;
	}
    // If request has not been created
    if (currRequest == nullptr) {
        // Peeks header
        uint8_t receivedHeader = peekByte();

        // Checks for incoming request header
        auto it = incomingByRequest.find(receivedHeader);
        if (it != incomingByRequest.end()) {
            // Creates new incoming request
            currRequest = std::make_shared<Request>(receivedHeader, Request::Type::INCOMING);
        }
        else if (getNumOutgoingRequests() > 0) {
            // Assigns to current outgoing request
            getOutgoingRequest(currRequest);
            // Sets reponse header
            currRequest->SetResponseHeader(receivedHeader);
        }
        else {
            // Header is unknown
            std::cerr << "(" + name + ") No packet found for received header " << int(receivedHeader) << std::endl;

            clearRequest();
            flush();
            return false;
        }

        // Determine expected response length for request
        int16_t expectedLength = 0;
        if (!getExpectedPacketLength(currRequest, expectedLength)) {
            if (currRequest->IsOutgoing()) {
                std::cerr << "(" + name + ") No response length found for outgoing request header " << int(currRequest->GetHeader()) << std::endl;
            }
            else {
                std::cerr << "(" + name + ") No packet length found for incoming request header " << int(currRequest->GetHeader()) << std::endl;
            }

            clearRequest();
            flush();
            return false;
        }
        currRequest->SetExpectedLength(expectedLength);
    }

    // Only process requests that have not yet been fufilled
    if (currRequest->IsComplete()) {
        return false;
    }

    // Gets packet length parameters
    if (!currRequest->IsCharacterized()) {
		std::size_t payloadLength = 0;
		std::size_t totalPacketLength = 0;
        if (!getPacketParameters(currRequest->GetExpectedLength(), payloadLength, totalPacketLength)) {
            // Waits until able to access all packet parameters
            return false;
        }
        else {
            currRequest->SetPayloadLength(payloadLength);
            currRequest->SetTotalPacketLength(totalPacketLength);
			currRequest->SetStatus(Request::Status::CHARACTERIZED);
        }
    }

    // Wait until the full packet is available
    if (getReadBufferSize() < currRequest->GetTotalPacketLength()) {
        return false;
    }

    // Now we have the full packet, so process it
    readByte(); // Removes header

    // If variable length, remove the length byte as well
    if (currRequest->GetExpectedLength() == -1) {
        readByte();
    }

    // Request is now complete
    currRequest->SetStatus(Request::Status::COMPLETE);

    // Adjust reserved bytes
    reservedBytes += currRequest->GetPayloadLength();
    return true;
}

void MinBiTCore::asyncFetchByte() {
    if (!stream || !stream->isOpen()) return;

    auto tempBuffer = std::make_shared<std::vector<uint8_t>>(1);
    stream->asyncRead(tempBuffer->data(), 1,
        [this, tempBuffer](const boost::system::error_code& error, std::size_t bytesTransferred) {
            if (!error) {
                appendToReadBuffer(tempBuffer->data(), bytesTransferred);

                // Process packets only when enough data is available
                while (getReadBufferSize() > 0) {
                    // Gets current request and characterizes it
                    if (!characterizePacket()) {
                        break;
                    }

                    // Calls read handler if exists
                    if (readHandler) {
                        readHandler(currRequest);
                    }
                    
                    // Clears request
                    clearRequest();
                }

                // Timeout check: remove outgoing requests that have timed out
                checkForTimeouts();

            }
            else {
                std::cerr << "(" + name + ") Error reading from stream: " << error.message() << std::endl;
            }
        });
}

uint8_t MinBiTCore::readByte() {
    // Read a single byte from the read buffer.
    uint8_t value;
    readBytes(&value, sizeof(value));
    return value;
}

uint8_t MinBiTCore::peekByte() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return readBuffer[0];
}

void MinBiTCore::readBytes(uint8_t* buffer, std::size_t len) {
    // Read a sequence of bytes from the read buffer.
    std::lock_guard<std::mutex> lock(dataMutex);
    if (readBuffer.size() < len) throw std::runtime_error("(" + name + ") Buffer underflow");
    std::memcpy(buffer, readBuffer.data(), len);
    readBuffer.erase(readBuffer.begin(), readBuffer.begin() + len);
	// Adjust reserved bytes
    if (reservedBytes >= len) {
        reservedBytes -= len;
    }
    else {
        reservedBytes = 0;
	}
}

int16_t MinBiTCore::readInt16() {
    uint8_t buffer[sizeof(int16_t)];
    readBytes(buffer, sizeof(int16_t));
    int16_t value;
    std::memcpy(&value, buffer, sizeof(int16_t));
    return value;
}

float MinBiTCore::readFloat() {
    // Read a float from the read buffer, handling endianness.
    uint8_t buffer[sizeof(float)];
    readBytes(buffer, sizeof(float));
    uint32_t networkValue;
    std::memcpy(&networkValue, buffer, sizeof(uint32_t));
    if (endianness == Endianness::BigEndian) {
        networkValue = boost::endian::big_to_native(networkValue);
    } else {
        networkValue = boost::endian::little_to_native(networkValue);
    }
    return *reinterpret_cast<float*>(&networkValue);
}

Eigen::Vector3d MinBiTCore::readVector3d() {
    // Read a 3D vector (three floats) from the read buffer.
    Eigen::Vector3d vector;
    for (uint8_t i = 0; i < 3; ++i) {
        vector(i) = readFloat();
    }
    return vector;
}

Eigen::Quaterniond MinBiTCore::readQuaterniond() {
    // Read a quaternion (four floats) from the read buffer.
    Eigen::Vector4d coeffs;
    for (uint8_t i = 0; i < 4; ++i) {
        coeffs(i) = readFloat();
    }
    return Eigen::Quaterniond(coeffs[3], coeffs[0], coeffs[1], coeffs[2]);
}

bool MinBiTCore::clearRequest() {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (currRequest != nullptr) {
        // Removes current request from queue if outgoing
        if (currRequest->IsOutgoing() && outgoingRequests.size() > 0) {
            outgoingRequests.pop();
        }
        // Clears current request
        currRequest.reset();

        return true;
    }
    return false;
}

bool MinBiTCore::getOutgoingRequest(std::shared_ptr<Request>& request) {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (!outgoingRequests.empty()) {
        request = outgoingRequests.front();
        return true;
    }
    request.reset();
    return false;
}

void MinBiTCore::flush() {
    // Clear the read buffer
    std::lock_guard<std::mutex> lock(dataMutex);
    readBuffer.clear();
}

std::size_t MinBiTCore::getReadBufferSize() {
    // Get the size of the read buffer (thread-safe).
    std::lock_guard<std::mutex> lock(dataMutex); // Ensure thread-safe access
    return readBuffer.size();
}

std::size_t MinBiTCore::getWriteBufferSize() {
    // Get the size of the write buffer (thread-safe).
    std::lock_guard<std::mutex> lock(dataMutex); // Ensure thread-safe access
    return writeBuffer.size();
}

std::size_t MinBiTCore::getNumOutgoingRequests() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return outgoingRequests.size();
}

std::size_t MinBiTCore::getReservedBytes() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return reservedBytes;
}

void MinBiTCore::appendToReadBuffer(const uint8_t* data, std::size_t length) {
    // Append data to the read buffer (thread-safe).
    std::lock_guard<std::mutex> lock(dataMutex);
    readBuffer.insert(readBuffer.end(), data, data + length);
}

void MinBiTCore::appendToWriteBuffer(const uint8_t* data, std::size_t length) {
    // Append data to the write buffer (thread-safe).
    std::lock_guard<std::mutex> lock(dataMutex);
    writeBuffer.insert(writeBuffer.end(), data, data + length);
}
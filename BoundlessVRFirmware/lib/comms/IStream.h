#ifndef ISTREAM_H
#define ISTREAM_H

class IStream {
    public:
        virtual ~IStream() = default;

        virtual void write(const uint8_t* buffer, std::size_t length);
        virtual void read(uint8_t* buffer, std::size_t length);
        virtual uint8_t available();
        virtual bool isOpen();
        virtual void close();
};

#endif // ISTREAM_H

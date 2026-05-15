#include "../src/H264Decoder.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>
#include <QTextStream>

#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    const QString captureDir = argc > 1
                                   ? QString::fromLocal8Bit(argv[1])
                                   : QStringLiteral("carplay/build/raw_capture");

    QDir dir(captureDir);
    const QFileInfoList entries = dir.entryInfoList({"*.h264"}, QDir::Files, QDir::Name);
    if (entries.isEmpty()) {
        err << "No .h264 capture frames found in " << dir.absolutePath() << Qt::endl;
        return 1;
    }

    H264Decoder decoder(30);
    int packets = 0;
    int decodedFrames = 0;

    for (const QFileInfo &entry : entries) {
        std::ifstream file(entry.absoluteFilePath().toStdString(), std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bytes.empty())
            continue;

        ++packets;
        const std::vector<QVideoFrame> frames = decoder.decode(bytes.data(), static_cast<qsizetype>(bytes.size()));
        decodedFrames += static_cast<int>(frames.size());
    }

    out << "Packets read: " << packets << Qt::endl;
    out << "Decoded frames: " << decodedFrames << Qt::endl;

    return decodedFrames > 0 ? 0 : 2;
}

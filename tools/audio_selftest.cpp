#include <QAudioDevice>
#include <QAudioSink>
#include <QBuffer>
#include <QCoreApplication>
#include <QMediaDevices>
#include <QTimer>

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

QString stateName(QAudio::State state)
{
    switch (state) {
    case QAudio::ActiveState:
        return QStringLiteral("active");
    case QAudio::SuspendedState:
        return QStringLiteral("suspended");
    case QAudio::StoppedState:
        return QStringLiteral("stopped");
    case QAudio::IdleState:
        return QStringLiteral("idle");
    }
    return QStringLiteral("unknown");
}

QString errorName(QAudio::Error error)
{
    switch (error) {
    case QAudio::NoError:
        return QStringLiteral("none");
    case QAudio::OpenError:
        return QStringLiteral("open");
    case QAudio::IOError:
        return QStringLiteral("io");
    case QAudio::UnderrunError:
        return QStringLiteral("underrun");
    case QAudio::FatalError:
        return QStringLiteral("fatal");
    }
    return QStringLiteral("unknown");
}

QByteArray makeSine(const QAudioFormat &format, int seconds)
{
    const int frames = format.sampleRate() * seconds;
    QByteArray pcm(frames * format.bytesPerFrame(), Qt::Uninitialized);
    auto *samples = reinterpret_cast<int16_t *>(pcm.data());

    for (int frame = 0; frame < frames; ++frame) {
        const double phase = 2.0 * M_PI * 700.0 * frame / format.sampleRate();
        const auto value = static_cast<int16_t>(std::sin(phase) * 12000);
        for (int channel = 0; channel < format.channelCount(); ++channel)
            *samples++ = value;
    }

    return pcm;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    std::cout << "Audio outputs: " << outputs.size() << '\n';
    for (const QAudioDevice &device : outputs) {
        std::cout << " - " << device.description().toStdString()
                  << " id=" << device.id().toStdString()
                  << (device.isDefault() ? " default" : "") << '\n';
    }

    const QString requested = argc > 1
                                  ? QString::fromLocal8Bit(argv[1])
                                  : qEnvironmentVariable("QTCARPLAY_AUDIO_OUTPUT_ID");
    QAudioDevice output = QMediaDevices::defaultAudioOutput();
    if (!requested.isEmpty()) {
        for (const QAudioDevice &device : outputs) {
            if (QString::fromUtf8(device.id()) == requested || device.description().contains(requested, Qt::CaseInsensitive)) {
                output = device;
                break;
            }
        }
    }

    if (output.isNull()) {
        std::cerr << "No default Qt audio output\n";
        return 1;
    }

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!output.isFormatSupported(format)) {
        std::cout << "44100/S16/stereo not reported as supported, using preferred format\n";
        format = output.preferredFormat();
    }

    std::cout << "Using: " << output.description().toStdString()
              << " id=" << output.id().toStdString()
              << " format=" << format.sampleRate()
              << "Hz/" << format.channelCount()
              << "ch bytesPerFrame=" << format.bytesPerFrame() << '\n';

    QByteArray pcm = makeSine(format, 3);
    QBuffer buffer(&pcm);
    buffer.open(QIODevice::ReadOnly);

    QAudioSink sink(output, format);
    QObject::connect(&sink, &QAudioSink::stateChanged, [&](QAudio::State state) {
        std::cout << "state=" << stateName(state).toStdString()
                  << " error=" << errorName(sink.error()).toStdString() << '\n';
        if (state == QAudio::IdleState || sink.error() == QAudio::FatalError)
            QTimer::singleShot(250, &app, &QCoreApplication::quit);
    });

    sink.setVolume(1.0);
    sink.start(&buffer);
    QTimer::singleShot(5000, &app, &QCoreApplication::quit);
    const int rc = app.exec();

    sink.stop();
    return sink.error() == QAudio::NoError || sink.error() == QAudio::UnderrunError ? rc : 2;
}

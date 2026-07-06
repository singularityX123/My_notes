// bdfparser.cpp
#include "bdfparser.h"
#include <QDebug>
#include <QDateTime>

namespace {
qint32 readSigned24(const QByteArray &sampleBytes)
{
    if (sampleBytes.size() < 3) {
        return 0;
    }

    qint32 value = (static_cast<unsigned char>(sampleBytes[0])) |
                   (static_cast<unsigned char>(sampleBytes[1]) << 8) |
                   (static_cast<unsigned char>(sampleBytes[2]) << 16);

    if (value & 0x800000) {
        value |= 0xFF000000;
    }

    return value;
}
}

BDFParser::BDFParser(QObject *parent) : QObject(parent) {}

void BDFParser::closeFile()
{
    if (bdfFile.isOpen()) {
        bdfFile.close();
    }

    header = BDFHeader{};
    dataStartPos = 0;
    currentPos = 0;
    currentSamplePosition = 0;
    bytesPerSample = 3;
    rawBuffer.clear();
    pendingSamples.clear();
}

bool BDFParser::openFile(const QString &filePath) {
    closeFile();
    bdfFile.setFileName(filePath);
    if (!bdfFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filePath;
        return false;
    }

    if (!parseHeader()) {
        qWarning() << "解析BDF头失败";
        return false;
    }

    // 定位到数据起始位置
    currentPos = dataStartPos;
    bdfFile.seek(currentPos);

    return true;
}

bool BDFParser::parseHeader() {
    header = BDFHeader{};

    QByteArray fixedHeader = bdfFile.read(256);
    if (fixedHeader.size() < 256) {
        return false;
    }

    header.patientID = QString::fromLatin1(fixedHeader.mid(8, 80)).trimmed();
    header.recordingID = QString::fromLatin1(fixedHeader.mid(88, 80)).trimmed();

    const QString dateStr = QString::fromLatin1(fixedHeader.mid(168, 8)).trimmed();
    const QString timeStr = QString::fromLatin1(fixedHeader.mid(176, 8)).trimmed();
    header.startDate = QDateTime::fromString(dateStr + " " + timeStr, "dd.MM.yy hh.mm.ss");

    const int headerBytes = QString::fromLatin1(fixedHeader.mid(184, 8)).trimmed().toInt();
    header.numRecords = QString::fromLatin1(fixedHeader.mid(236, 8)).trimmed().toInt();
    header.recordDuration = QString::fromLatin1(fixedHeader.mid(244, 8)).trimmed().toDouble();
    header.numChannels = QString::fromLatin1(fixedHeader.mid(252, 4)).trimmed().toInt();

    if (header.numChannels <= 0) {
        return false;
    }

    const int signalHeaderBytes = header.numChannels * 256;
    QByteArray signalHeaderData = bdfFile.read(signalHeaderBytes);
    if (signalHeaderData.size() < signalHeaderBytes) {
        return false;
    }

    header.channelLabels.clear();
    header.samplesPerRecord.clear();
    header.channelGains.clear();
    header.channelOffsets.clear();

    const int n = header.numChannels;
    const int labelsOffset = 0;
    const int physMinOffset = (16 + 80 + 8) * n;
    const int physMaxOffset = (16 + 80 + 8 + 8) * n;
    const int digMinOffset = (16 + 80 + 8 + 8 + 8) * n;
    const int digMaxOffset = (16 + 80 + 8 + 8 + 8 + 8) * n;
    const int samplesOffset = (16 + 80 + 8 + 8 + 8 + 8 + 8 + 80) * n;

    for (int i = 0; i < n; ++i) {
        const QString label = QString::fromLatin1(signalHeaderData.mid(labelsOffset + i * 16, 16)).trimmed();
        header.channelLabels.append(label);

        bool okPhysMin = false;
        bool okPhysMax = false;
        bool okDigMin = false;
        bool okDigMax = false;
        bool okSpr = false;

        const double physMin = QString::fromLatin1(signalHeaderData.mid(physMinOffset + i * 8, 8)).trimmed().toDouble(&okPhysMin);
        const double physMax = QString::fromLatin1(signalHeaderData.mid(physMaxOffset + i * 8, 8)).trimmed().toDouble(&okPhysMax);
        const double digMin = QString::fromLatin1(signalHeaderData.mid(digMinOffset + i * 8, 8)).trimmed().toDouble(&okDigMin);
        const double digMax = QString::fromLatin1(signalHeaderData.mid(digMaxOffset + i * 8, 8)).trimmed().toDouble(&okDigMax);
        const int samplesPerRecord = QString::fromLatin1(signalHeaderData.mid(samplesOffset + i * 8, 8)).trimmed().toInt(&okSpr);

        header.samplesPerRecord.append(okSpr ? samplesPerRecord : 0);

        double gain = 1.0;
        int offset = 0;
        if (okPhysMin && okPhysMax && okDigMin && okDigMax && std::abs(digMax - digMin) > 1e-9) {
            gain = (physMax - physMin) / (digMax - digMin);
            offset = static_cast<int>(std::round(physMin - gain * digMin));
        }

        header.channelGains.append(gain);
        header.channelOffsets.append(offset);
    }

    const int refIndex = referenceChannelIndex();
    const int refSamplesPerRecord = (refIndex >= 0 && refIndex < header.samplesPerRecord.size())
                                        ? header.samplesPerRecord.at(refIndex)
                                        : 0;

    header.sampleRate = (header.recordDuration > 0.0)
                            ? static_cast<int>(std::round(refSamplesPerRecord / header.recordDuration))
                            : refSamplesPerRecord;
    header.duration = (header.numRecords > 0 && header.recordDuration > 0.0)
                          ? static_cast<int>(std::round(header.numRecords * header.recordDuration))
                          : 0;

    if (headerBytes > 0) {
        dataStartPos = headerBytes;
    } else {
        dataStartPos = 256 + signalHeaderBytes;
    }

    const qint64 dataBytes = qMax<qint64>(0, bdfFile.size() - dataStartPos);
    qint64 bytesPerRecord = 0;
    for (int i = 0; i < header.samplesPerRecord.size(); ++i) {
        bytesPerRecord += static_cast<qint64>(header.samplesPerRecord.at(i)) * bytesPerSample;
    }

    const qint64 samplesInFile = (refSamplesPerRecord > 0 && bytesPerRecord > 0)
                                     ? (dataBytes / bytesPerRecord) * refSamplesPerRecord
                                     : 0;
    if (header.numRecords > 0 && refSamplesPerRecord > 0) {
        header.numSamples = header.numRecords * refSamplesPerRecord;
    } else {
        header.numSamples = static_cast<int>(samplesInFile);
    }

    bytesPerSample = 3; // BDF是24位整数，3字节

    qDebug() << "采样率:" << header.sampleRate << "Hz";
    qDebug() << "通道数:" << header.numChannels;
    qDebug() << "通道标签:" << header.channelLabels;

    return true;
}

void BDFParser::resetPosition()
{
    if (bdfFile.isOpen()) {
        bdfFile.seek(dataStartPos);
    }
    currentPos = dataStartPos;
    currentSamplePosition = 0;
    pendingSamples = QVector<QVector<double>>(header.numChannels);
}

qint64 BDFParser::getCurrentPosition() const
{
    return qMax<qint64>(0, currentSamplePosition);
}

bool BDFParser::seekToSample(qint64 sampleIndex)
{
    if (!bdfFile.isOpen() || sampleIndex < 0) {
        return false;
    }

    const int refIndex = referenceChannelIndex();
    if (refIndex < 0 || refIndex >= header.samplesPerRecord.size()) {
        return false;
    }

    const int refSamplesPerRecord = header.samplesPerRecord.at(refIndex);
    if (refSamplesPerRecord <= 0) {
        return false;
    }

    qint64 bytesPerRecord = 0;
    for (int i = 0; i < header.samplesPerRecord.size(); ++i) {
        bytesPerRecord += static_cast<qint64>(header.samplesPerRecord.at(i)) * bytesPerSample;
    }

    if (bytesPerRecord <= 0) {
        return false;
    }

    const qint64 maxSample = qMax<qint64>(0, header.numSamples - 1);
    sampleIndex = qBound<qint64>(0, sampleIndex, maxSample);

    const qint64 recordIndex = sampleIndex / refSamplesPerRecord;
    const int inRecordOffset = static_cast<int>(sampleIndex % refSamplesPerRecord);
    const qint64 seekPos = dataStartPos + recordIndex * bytesPerRecord;

    if (!bdfFile.seek(seekPos)) {
        return false;
    }

    currentPos = seekPos;
    currentSamplePosition = recordIndex * refSamplesPerRecord;
    pendingSamples = QVector<QVector<double>>(header.numChannels);

    if (inRecordOffset > 0) {
        if (!readDataRecord()) {
            return false;
        }

        for (int ch = 0; ch < pendingSamples.size(); ++ch) {
            const int samplesInChannelRecord = (ch < header.samplesPerRecord.size()) ? header.samplesPerRecord.at(ch) : refSamplesPerRecord;
            if (samplesInChannelRecord <= 0) {
                continue;
            }

            const int channelOffset = static_cast<int>((static_cast<double>(inRecordOffset) * samplesInChannelRecord) / refSamplesPerRecord);
            const int safeOffset = qBound(0, channelOffset, pendingSamples[ch].size());
            if (safeOffset > 0) {
                pendingSamples[ch].remove(0, safeOffset);
            }
        }

        currentSamplePosition += inRecordOffset;
    }

    return true;
}

double BDFParser::getTotalDuration() const
{
    if (header.sampleRate <= 0) {
        return 0.0;
    }

    return static_cast<double>(header.numSamples) / header.sampleRate;
}

QVector<QVector<double>> BDFParser::getDataWindow(double startTime, double windowSize)
{
    if (!bdfFile.isOpen() || header.numChannels <= 0 || header.sampleRate <= 0) {
        return {};
    }

    const qint64 savedPos = bdfFile.pos();
    const qint64 savedCurrentPos = currentPos;
    const qint64 savedCurrentSamplePos = currentSamplePosition;
    const QVector<QVector<double>> savedPending = pendingSamples;

    resetPosition();

    const qint64 startSample = static_cast<qint64>(startTime * header.sampleRate);
    if (startSample > 0) {
        getNextBlock(static_cast<int>(startSample));
    }

    QVector<QVector<double>> windowData = getNextBlock(static_cast<int>(windowSize * header.sampleRate));

    if (savedPos >= 0) {
        bdfFile.seek(savedPos);
    }
    currentPos = savedCurrentPos;
    currentSamplePosition = savedCurrentSamplePos;
    pendingSamples = savedPending;

    return windowData;
}

QVector<double> BDFParser::getChannelData(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= header.numChannels) {
        return {};
    }

    const qint64 savedPos = bdfFile.pos();
    const qint64 savedCurrentPos = currentPos;

    resetPosition();
    QVector<QVector<double>> allData = getNextBlock(header.numSamples > 0 ? header.numSamples : 0);

    if (savedPos >= 0) {
        bdfFile.seek(savedPos);
    }
    currentPos = savedCurrentPos;

    if (channelIndex >= allData.size()) {
        return {};
    }

    return allData[channelIndex];
}

QVector<QVector<double>> BDFParser::getNextBlock(int blockSize)
{
    if (!bdfFile.isOpen() || header.numChannels <= 0 || blockSize <= 0) {
        return {};
    }

    if (pendingSamples.size() != header.numChannels) {
        pendingSamples = QVector<QVector<double>>(header.numChannels);
    }

    const int refIndex = referenceChannelIndex();
    if (refIndex < 0 || refIndex >= pendingSamples.size()) {
        return {};
    }

    while (pendingSamples.at(refIndex).size() < blockSize) {
        if (!readDataRecord()) {
            break;
        }
    }

    const int available = pendingSamples.at(refIndex).size();
    if (available <= 0) {
        return {};
    }

    const int outputSamples = qMin(blockSize, available);

    QVector<QVector<double>> block(header.numChannels);
    for (int ch = 0; ch < header.numChannels; ++ch) {
        const int takeCount = qMin(outputSamples, pendingSamples[ch].size());
        if (takeCount <= 0) {
            continue;
        }

        block[ch] = pendingSamples[ch].mid(0, takeCount);
        pendingSamples[ch].remove(0, takeCount);
    }

    currentPos = bdfFile.pos();
    currentSamplePosition += outputSamples;
    return block;
}

bool BDFParser::readDataRecord()
{
    if (!bdfFile.isOpen()) {
        return false;
    }

    if (pendingSamples.size() != header.numChannels) {
        pendingSamples = QVector<QVector<double>>(header.numChannels);
    }

    for (int ch = 0; ch < header.numChannels; ++ch) {
        const int samplesInThisRecord = (ch < header.samplesPerRecord.size()) ? header.samplesPerRecord.at(ch) : 0;
        for (int i = 0; i < samplesInThisRecord; ++i) {
            const QByteArray sampleBytes = bdfFile.read(bytesPerSample);
            if (sampleBytes.size() < bytesPerSample) {
                currentPos = bdfFile.pos();
                return false;
            }

            const qint32 rawValue = readSigned24(sampleBytes);
            const double gain = (ch < header.channelGains.size()) ? header.channelGains.at(ch) : 1.0;
            const double offset = (ch < header.channelOffsets.size()) ? header.channelOffsets.at(ch) : 0.0;
            pendingSamples[ch].append(rawValue * gain + offset);
        }
    }

    currentPos = bdfFile.pos();
    return true;
}

int BDFParser::referenceChannelIndex() const
{
    int bestIndex = -1;
    int bestSamples = 0;

    for (int i = 0; i < header.channelLabels.size() && i < header.samplesPerRecord.size(); ++i) {
        const QString label = header.channelLabels.at(i).toLower();
        const int spr = header.samplesPerRecord.at(i);
        const bool annotationChannel = label.contains("annotation");
        if (!annotationChannel && spr > bestSamples) {
            bestSamples = spr;
            bestIndex = i;
        }
    }

    if (bestIndex >= 0) {
        return bestIndex;
    }

    for (int i = 0; i < header.samplesPerRecord.size(); ++i) {
        if (header.samplesPerRecord.at(i) > bestSamples) {
            bestSamples = header.samplesPerRecord.at(i);
            bestIndex = i;
        }
    }

    return bestIndex;
}

int BDFParser::getChannelIndex(const QString &label) {
    for (int i = 0; i < header.channelLabels.size(); i++) {
        if (header.channelLabels[i].contains(label, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

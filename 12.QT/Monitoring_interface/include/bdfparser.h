// bdfparser.h
#ifndef BDFPARSER_H
#define BDFPARSER_H

#include <QObject>
#include <QFile>
#include <QVector>
#include <QDateTime>

struct BDFHeader {
    QString patientID;
    QString recordingID;
    QDateTime startDate;
    int numChannels;        // 通道数
    int numRecords;         // 数据记录数
    double recordDuration;  // 每条数据记录时长(秒)
    int sampleRate;         // 采样率
    int numSamples;         // 总采样点数
    int duration;           // 时长(秒)
    QVector<QString> channelLabels;  // 通道标签
    QVector<int> samplesPerRecord;   // 每通道每条记录采样点
    QVector<double> channelGains;    // 通道增益
    QVector<int> channelOffsets;     // 通道偏移
};

class BDFParser : public QObject {
    Q_OBJECT

public:
    explicit BDFParser(QObject *parent = nullptr);
    bool openFile(const QString &filePath);
    void closeFile();

    // 获取指定时间窗口的数据
    QVector<QVector<double>> getDataWindow(double startTime, double windowSize);

    // 获取指定通道的数据
    QVector<double> getChannelData(int channelIndex);

    // 获取所有通道的最新数据块
    QVector<QVector<double>> getNextBlock(int blockSize);

    // 获取通道索引(根据名称)
    int getChannelIndex(const QString &label);

    // 获取头部信息
    BDFHeader getHeader() const { return header; }

    // 获取文件总时长
    double getTotalDuration() const;

    // 重置读取位置
    void resetPosition();

    // 获取当前读取到第几个采样点
    qint64 getCurrentPosition() const;

    // 跳转到指定采样点位置(参考通道)
    bool seekToSample(qint64 sampleIndex);

private:
    bool parseHeader();
    bool readDataRecord();
    int referenceChannelIndex() const;

    BDFHeader header;
    QFile bdfFile;
    qint64 dataStartPos;        // 数据起始位置
    qint64 currentPos;          // 当前读取位置
    qint64 currentSamplePosition; // 参考通道采样点位置
    int bytesPerSample;          // 每采样点字节数
    QVector<qint16> rawBuffer;   // 原始数据缓冲
    QVector<QVector<double>> pendingSamples; // 按通道缓存未消费数据
};

#endif

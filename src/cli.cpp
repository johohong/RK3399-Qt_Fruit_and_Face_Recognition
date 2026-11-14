#include "detector.h"
#include "io.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    addDetectorArguments(parser);
    parser.addOption({"image", "Absolute image path", "path"});
    parser.addOption({"output-json", "Absolute JSON output path", "path"});
    parser.addOption({"output-image", "Absolute annotated image path", "path"});
    parser.addOption({"warmup", "Warmup iterations", "count", "0"});
    parser.addOption({"iterations", "Measured iterations", "count", "1"});
    if (!parser.parse(app.arguments())) { qCritical() << parser.errorText(); return 2; }
    if (parser.isSet("help")) parser.showHelp();
    DetectorOptions options;
    QString error;
    if (!parseDetectorArguments(parser, options, error)) { qCritical().noquote() << error; return 2; }
    for (auto key : {"image", "output-json"}) {
        if (!QFileInfo(parser.value(key)).isAbsolute()) { qCritical() << "ARGUMENT_INVALID" << key; return 2; }
    }
    if (parser.isSet("output-image") && !QFileInfo(parser.value("output-image")).isAbsolute()) return 2;
    bool warmupOk=false, iterationsOk=false;
    int warmup=parser.value("warmup").toInt(&warmupOk), iterations=parser.value("iterations").toInt(&iterationsOk);
    if (!warmupOk || !iterationsOk || warmup<0 || warmup>10000 || iterations<1 || iterations>10000) return 2;
    auto detector=createDetector();
    if (!detector->load(options,error)) { qCritical().noquote()<<error; return 3; }
    cv::Mat image;
    try { image=readImage(parser.value("image")); }
    catch (const std::exception& e) { qCritical()<<e.what(); return 4; }
    if (image.empty()) { qCritical()<<"IMAGE_UNREADABLE"; return 4; }
    for (int i=0; i<warmup; ++i) { auto r=detector->detect(image); if (!r.ok) { qCritical()<<r.error; return 5; } }
    DetectionResult result;
    QJsonArray timings;
    std::vector<double> totals;
    for (int i=0; i<iterations; ++i) {
        result=detector->detect(image);
        if (!result.ok) break;
        timings.append(resultJson(result)["timing_ms"]);
        totals.push_back(result.totalMs);
    }
    QJsonObject json=resultJson(result);
    json["model"]=options.modelPath;
    json["image"]=parser.value("image");
    json["width"]=image.cols;
    json["height"]=image.rows;
    json["threads"]=options.threads;
    json["warmup"]=warmup;
    json["samples"]=timings;
    if (!totals.empty()) {
        std::sort(totals.begin(),totals.end());
        auto percentile=[&](double fraction) { double index=(totals.size()-1)*fraction; size_t low=size_t(index), high=std::min(low+1,totals.size()-1); return totals[low]+(totals[high]-totals[low])*(index-low); };
        json["total_ms_p50"]=percentile(.5);
        json["total_ms_p95"]=percentile(.95);
    }
    QFile output(parser.value("output-json"));
    QByteArray data=QJsonDocument(json).toJson();
    if (!output.open(QIODevice::WriteOnly) || output.write(data)!=data.size()) { qCritical()<<"OUTPUT_UNWRITABLE"; return 4; }
    if (!result.ok) { qCritical().noquote()<<result.error; return 5; }
    if (parser.isSet("output-image") && !writeImage(parser.value("output-image"),annotate(image,result))) return 4;
    qInfo()<<"DETECTION_PASS"<<result.detections.size()<<result.totalMs<<"ms";
    return 0;
}

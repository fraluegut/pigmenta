#include "Engine.h"

#include "pigmenta/decorrelation.h"
#include "pigmenta/filters.h"
#include "Translator.h"

namespace pigmenta::desktop {

namespace {

ParamSpec colorspaceParam() {
    ParamSpec p;
    p.name = "colorspace";
    p.label = Translator::t("param.colorspace");
    p.kind = ParamKind::Choice;
    p.choices = {"YCbCr", "Lab", "RGB", "YRD", "VDS"};
    p.defaultValue = 1;  // Lab
    return p;
}

ParamSpec floatParam(
    const QString& name, const QString& label, double lo, double hi, double step, double def) {
    ParamSpec p;
    p.name = name;
    p.label = label;
    p.kind = ParamKind::Float;
    p.min = lo;
    p.max = hi;
    p.step = step;
    p.defaultValue = def;
    return p;
}

::pigmenta::ColorSpace toColorSpace(double choiceIndex) {
    switch (static_cast<int>(choiceIndex)) {
        case 0:
            return ::pigmenta::ColorSpace::YCBCR;
        case 2:
            return ::pigmenta::ColorSpace::RGB;
        case 3:
            return ::pigmenta::ColorSpace::YRD;
        case 4:
            return ::pigmenta::ColorSpace::VDS;
        default:
            return ::pigmenta::ColorSpace::LAB;
    }
}

}  // namespace

const std::vector<FilterSpec>& filterSpecs() {
    // Rebuilt (cheap) on every call rather than cached, so it always
    // reflects the current language.
    static std::vector<FilterSpec> specs;
    specs = {
        {FilterId::Original, Translator::t("filter.original"), {}},
        {FilterId::Decorrelation,
         Translator::t("filter.decorrelation"),
         {
             colorspaceParam(),
             floatParam("targetStd", Translator::t("param.targetStd"), 5.0, 100.0, 1.0, 45.0),
             floatParam("intensity", Translator::t("param.intensity"), 0.0, 1.0, 0.01, 1.0),
         }},
        {FilterId::Clahe,
         Translator::t("filter.clahe"),
         {
             floatParam("tileSize", Translator::t("param.tileSize"), 4.0, 64.0, 1.0, 16.0),
             floatParam("clipLimit", Translator::t("param.clipLimit"), 0.5, 8.0, 0.1, 2.0),
         }},
        {FilterId::Gamma,
         Translator::t("filter.gamma"),
         {
             floatParam("gamma", Translator::t("param.gamma"), 0.2, 3.0, 0.05, 1.0),
         }},
        {FilterId::Saturation,
         Translator::t("filter.saturation"),
         {
             floatParam("factor", Translator::t("param.factor"), 0.0, 4.0, 0.05, 2.0),
         }},
        {FilterId::Relief,
         Translator::t("filter.relief"),
         {
             floatParam("angle", Translator::t("param.angle"), 0.0, 360.0, 1.0, 315.0),
             floatParam("strength", Translator::t("param.strength"), 0.0, 1.0, 0.01, 0.5),
         }},
        {FilterId::IrSimulate,
         Translator::t("filter.irSimulate"),
         {
             floatParam("intensity", Translator::t("param.intensity"), 0.0, 1.0, 0.01, 0.8),
         }},
    };
    return specs;
}

QImage applyFilter(const QImage& src, FilterId filterId, const QMap<QString, double>& values) {
    QImage img = src.convertToFormat(QImage::Format_RGBA8888);
    if (filterId == FilterId::Original)
        return img;

    auto* pixels = reinterpret_cast<uint32_t*>(img.bits());
    const int width = img.width();
    const int height = img.height();
    const int stride = static_cast<int>(img.bytesPerLine());

    switch (filterId) {
        case FilterId::Decorrelation:
            ::pigmenta::decorrelation_stretch(pixels,
                                              width,
                                              height,
                                              stride,
                                              toColorSpace(values.value("colorspace", 1.0)),
                                              static_cast<float>(values.value("targetStd", 45.0)),
                                              static_cast<float>(values.value("intensity", 1.0)));
            break;
        case FilterId::Clahe:
            ::pigmenta::clahe_lab(pixels,
                                  width,
                                  height,
                                  stride,
                                  static_cast<int>(values.value("tileSize", 16.0)),
                                  static_cast<float>(values.value("clipLimit", 2.0)));
            break;
        case FilterId::Gamma:
            ::pigmenta::gamma_correct(
                pixels, width, height, stride, static_cast<float>(values.value("gamma", 1.0)));
            break;
        case FilterId::Saturation:
            ::pigmenta::saturation_boost(
                pixels, width, height, stride, static_cast<float>(values.value("factor", 2.0)));
            break;
        case FilterId::Relief:
            ::pigmenta::relief_shade(pixels,
                                     width,
                                     height,
                                     stride,
                                     static_cast<float>(values.value("angle", 315.0)),
                                     static_cast<float>(values.value("strength", 0.5)));
            break;
        case FilterId::IrSimulate:
            ::pigmenta::ir_simulate(
                pixels, width, height, stride, static_cast<float>(values.value("intensity", 0.8)));
            break;
    }
    return img;
}

}  // namespace pigmenta::desktop

/*
    SPDX-FileCopyrightText: 2024 Contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ekos/focus/donutmetrics.h"

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtTest/QTest>
#else
#include <QTest>
#endif

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

using Ekos::FocusUtils::DonutAnalysisResult;
using Ekos::FocusUtils::computeDonutAnalysis;

namespace
{
FITSImage::Statistic makeStats(int width, int height, const std::vector<double> &buffer, double forcedStdDev = -1.0)
{
    FITSImage::Statistic stats;
    stats.width = width;
    stats.height = height;
    stats.samples_per_channel = width * height;

    if (!buffer.empty())
    {
        const double sum = std::accumulate(buffer.begin(), buffer.end(), 0.0);
        const double mean = sum / buffer.size();
        stats.mean[0] = mean;

        double variance = 0.0;
        for (const double value : buffer)
            variance += (value - mean) * (value - mean);

        const double sigma = forcedStdDev > 0.0 ? forcedStdDev : std::sqrt(variance / buffer.size());
        stats.stddev[0] = sigma > 0.0 ? sigma : 1.0;
    }
    else
    {
        stats.mean[0] = 0.0;
        stats.stddev[0] = 1.0;
    }

    return stats;
}

std::vector<double> buildSyntheticDonut(int width, int height, double background, double ringPeak, double centreDip)
{
    std::vector<double> buffer(width * height, background);
    const double cx = width / 2.0;
    const double cy = height / 2.0;
    const double innerHole = 6.0;
    const double ringInner = 12.0;
    const double ringOuter = 16.0;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const double radius = std::hypot(x - cx, y - cy);
            const int index = y * width + x;
            if (radius < innerHole)
                buffer[index] = std::max(0.0, background - centreDip);
            else if (radius >= ringInner && radius <= ringOuter)
                buffer[index] = background + ringPeak;
        }
    }

    return buffer;
}
}

class TestDonutMetric : public QObject
{
        Q_OBJECT

    private slots:
        void syntheticDonutProducesMetric();
        void rejectsDegenerateInput();
        void weightRespondsToContrast();
        void unsignedShortInput();
};

void TestDonutMetric::syntheticDonutProducesMetric()
{
    constexpr int width = 64;
    constexpr int height = 64;
    auto image = buildSyntheticDonut(width, height, 120.0, 700.0, 80.0);
    auto stats = makeStats(width, height, image, 12.0);

    const DonutAnalysisResult result = computeDonutAnalysis(image.data(), stats);
    QVERIFY(result.valid);
    QVERIFY(result.metric > 5.0);
    QVERIFY(result.metric < 30.0);
    QVERIFY(result.weight > 1.0);
}

void TestDonutMetric::rejectsDegenerateInput()
{
    FITSImage::Statistic stats;
    std::vector<double> empty;
    const DonutAnalysisResult nullResult = computeDonutAnalysis(static_cast<const double *>(nullptr), stats);
    QVERIFY(!nullResult.valid);

    stats.width = 4;
    stats.height = 4;
    stats.samples_per_channel = 16;
    stats.mean[0] = 0.0;
    stats.stddev[0] = 1.0;
    std::vector<double> tiny(stats.samples_per_channel, 10.0);
    const DonutAnalysisResult tinyResult = computeDonutAnalysis(tiny.data(), stats);
    QVERIFY(!tinyResult.valid);
}

void TestDonutMetric::weightRespondsToContrast()
{
    constexpr int width = 64;
    constexpr int height = 64;
    const auto lowContrast = buildSyntheticDonut(width, height, 100.0, 30.0, 5.0);
    const auto highContrast = buildSyntheticDonut(width, height, 100.0, 400.0, 5.0);

    auto lowStats = makeStats(width, height, lowContrast, 15.0);
    auto highStats = makeStats(width, height, highContrast, 15.0);

    const DonutAnalysisResult low = computeDonutAnalysis(lowContrast.data(), lowStats);
    const DonutAnalysisResult high = computeDonutAnalysis(highContrast.data(), highStats);

    QVERIFY(low.valid);
    QVERIFY(high.valid);
    QVERIFY2(high.weight > low.weight, "High contrast donut should yield a larger weight");
}

void TestDonutMetric::unsignedShortInput()
{
    constexpr int width = 64;
    constexpr int height = 64;
    const auto reference = buildSyntheticDonut(width, height, 90.0, 250.0, 15.0);

    std::vector<uint16_t> image(reference.size());
    std::transform(reference.begin(), reference.end(), image.begin(), [](double value)
    {
        const double clamped = std::clamp(value, 0.0, 65535.0);
        return static_cast<uint16_t>(std::lround(clamped));
    });

    auto stats = makeStats(width, height, reference, 10.0);
    const DonutAnalysisResult result = computeDonutAnalysis(image.data(), stats);
    QVERIFY(result.valid);
    QVERIFY(result.metric > 0.0);
}

QTEST_GUILESS_MAIN(TestDonutMetric)

#include "testdonutmetric.moc"

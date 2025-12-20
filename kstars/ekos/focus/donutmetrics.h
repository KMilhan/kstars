/*
    SPDX-FileCopyrightText: 2024 Contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "fitsviewer/structuredefinitions.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Ekos
{
namespace FocusUtils
{
struct DonutAnalysisResult
{
    bool valid = false;
    double metric = 0.0;
    double weight = 1.0;
    int innerRadius = 0;
    int outerRadius = 0;
    double ringContrast = 0.0;
};

template <typename T>
DonutAnalysisResult computeDonutAnalysis(const T *buffer, const FITSImage::Statistic &stats)
{
    DonutAnalysisResult result;

    if (buffer == nullptr)
        return result;

    const int width = stats.width;
    const int height = stats.height;
    const int stride = stats.samples_per_channel;

    if (width <= 0 || height <= 0 || stride <= 0)
        return result;

    const T *channel = buffer;
    const int sampleCount = stride;

    double peakValue = std::numeric_limits<double>::lowest();
    int peakIndex = -1;

    for (int i = 0; i < sampleCount; ++i)
    {
        const double value = static_cast<double>(channel[i]);
        if (!std::isfinite(value))
            continue;

        if (value > peakValue)
        {
            peakValue = value;
            peakIndex = i;
        }
    }

    if (peakIndex < 0 || peakValue <= 0 || !std::isfinite(peakValue))
        return result;

    int centreX = peakIndex % width;
    int centreY = peakIndex / width;

    double background = stats.mean[0];
    if (!std::isfinite(background))
        background = 0.0;

    double threshold = background + 0.5 * (peakValue - background);
    if (threshold >= peakValue)
        threshold = peakValue * 0.8;

    double sumX = 0;
    double sumY = 0;
    double sumValue = 0;

    for (int i = 0; i < sampleCount; ++i)
    {
        const double value = static_cast<double>(channel[i]);
        if (value >= threshold)
        {
            const int x = i % width;
            const int y = i / width;
            sumX += x * value;
            sumY += y * value;
            sumValue += value;
        }
    }

    if (sumValue > 0)
    {
        centreX = std::clamp(static_cast<int>(std::lround(sumX / sumValue)), 0, width - 1);
        centreY = std::clamp(static_cast<int>(std::lround(sumY / sumValue)), 0, height - 1);
    }

    const int marginX = std::min(centreX, width - 1 - centreX);
    const int marginY = std::min(centreY, height - 1 - centreY);
    int maxRadius = std::min({marginX, marginY, std::min(width, height) / 3});

    if (maxRadius < 5)
        return result;

    maxRadius = std::min(maxRadius, 128);

    std::vector<double> radialSum(maxRadius + 1, 0.0);
    std::vector<int> radialCount(maxRadius + 1, 0);

    for (int dy = -maxRadius; dy <= maxRadius; ++dy)
    {
        const int y = centreY + dy;
        if (y < 0 || y >= height)
            continue;

        for (int dx = -maxRadius; dx <= maxRadius; ++dx)
        {
            const int x = centreX + dx;
            if (x < 0 || x >= width)
                continue;

            const double radius = std::hypot(dx, dy);
            const int radiusIndex = static_cast<int>(std::lround(radius));
            if (radiusIndex > maxRadius)
                continue;

            const int index = y * width + x;
            const double value = static_cast<double>(channel[index]);
            radialSum[radiusIndex] += value;
            radialCount[radiusIndex] += 1;
        }
    }

    if (radialCount[0] > 0)
        radialSum[0] /= radialCount[0];

    for (int r = 1; r <= maxRadius; ++r)
    {
        if (radialCount[r] > 0)
            radialSum[r] /= radialCount[r];
        else
            radialSum[r] = radialSum[r - 1];
    }

    std::vector<double> smoothProfile(radialSum.size(), 0.0);
    const int smoothWindow = 2;

    for (int r = 0; r <= maxRadius; ++r)
    {
        double sum = 0;
        int count = 0;
        for (int offset = -smoothWindow; offset <= smoothWindow; ++offset)
        {
            const int idx = r + offset;
            if (idx < 0 || idx > maxRadius)
                continue;
            sum += radialSum[idx];
            ++count;
        }
        smoothProfile[r] = (count > 0) ? sum / count : radialSum[r];
    }

    const int searchStart = std::min(smoothWindow + 1, maxRadius);
    int outerPeak = searchStart;
    double outerValue = smoothProfile[outerPeak];

    for (int r = searchStart + 1; r <= maxRadius; ++r)
    {
        if (smoothProfile[r] > outerValue)
        {
            outerValue = smoothProfile[r];
            outerPeak = r;
        }
    }

    if (outerPeak <= 0)
        return result;

    int innerMin = 0;
    double innerValue = smoothProfile[0];

    for (int r = 1; r < outerPeak; ++r)
    {
        if (smoothProfile[r] < innerValue)
        {
            innerValue = smoothProfile[r];
            innerMin = r;
        }
    }

    if (outerPeak <= innerMin)
        return result;

    const double ringContrast = outerValue - innerValue;
    if (!std::isfinite(ringContrast) || ringContrast <= 0)
        return result;

    const double outerSq = static_cast<double>(outerPeak) * outerPeak;
    const double innerSq = static_cast<double>(innerMin) * innerMin;
    if (outerSq <= innerSq)
        return result;

    const double metric = std::sqrt(outerSq - innerSq);
    if (!std::isfinite(metric) || metric <= 0)
        return result;

    double noise = stats.stddev[0];
    if (!std::isfinite(noise) || noise <= 0)
        noise = 1.0;

    const double relativeContrast = ringContrast / (noise * 5.0);

    result.valid = true;
    result.metric = metric;
    result.weight = std::clamp(relativeContrast, 0.1, 5.0);
    result.innerRadius = innerMin;
    result.outerRadius = outerPeak;
    result.ringContrast = ringContrast;

    return result;
}
}
}


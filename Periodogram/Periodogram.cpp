// Copyright (c) 2014-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PeriodogramDisplay.hpp"
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>

/***********************************************************************
 * |PothosDoc Periodogram
 *
 * The periodogram plot displays a live two dimensional plot of power vs frequency.
 *
 * |category /Plotters
 * |keywords frequency plot fft dft spectrum spectral
 * |alias /widgets/periodogram
 *
 * |param title The title of the plot
 * |default "Power vs Frequency"
 * |widget StringEntry()
 * |preview valid
 *
 * |param numInputs[Num Inputs] The number of input ports.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 *
 * |param displayRate[Display Rate] How often the plotter updates.
 * |default 10.0
 * |units updates/sec
 * |preview disable
 *
 * |param sampleRate[Sample Rate] The rate of the input elements.
 * |default 1e6
 * |units samples/sec
 *
 * |param centerFreq[Center Freq] The center frequency of the plot.
 * This value controls the labeling of the horizontal access.
 * |default 0.0
 * |units Hz
 * |preview valid
 *
 * |param numBins[Num FFT Bins] The number of bins per fourier transform.
 * |default 1024
 * |option 512
 * |option 1024
 * |option 2048
 * |option 4096
 * |widget ComboBox(editable=true)
 * |preview disable
 * |tab FFT
 *
 * |param window[Window Type] The window function controls passband ripple.
 * |default "hann"
 * |option [Rectangular] "rectangular"
 * |option [Hann] "hann"
 * |option [Hamming] "hamming"
 * |option [Blackman] "blackman"
 * |option [Bartlett] "bartlett"
 * |option [Flat-top] "flattop"
 * |option [Kaiser] "kaiser"
 * |option [Chebyshev] "chebyshev"
 * |preview disable
 * |tab FFT
 *
 * |param windowArgs[Window Args] Optional window arguments (depends on window type).
 * <ul>
 * <li>When using the <i>Kaiser</i> window, specify [beta] to use the parameterized Kaiser window.</li>
 * <li>When using the <i>Chebyshev</i> window, specify [atten] to use the Dolph-Chebyshev window with attenuation in dB.</li>
 * </ul>
 * |default []
 * |preview disable
 * |tab FFT
 *
 * |param autoScale[Auto-Scale] Enable automatic scaling for the vertical axis.
 * |default false
 * |option [Auto scale] true
 * |option [Use limits] false
 * |preview disable
 * |tab Axis
 *
 * |param refLevel[Reference Level] The maximum displayable power level.
 * |default 0.0
 * |units dBxx
 * |widget DoubleSpinBox(minimum=-150, maximum=150, step=10, decimals=1)
 * |preview disable
 * |tab Axis
 *
 * |param dynRange[Dynamic Range] The ratio of largest to smallest displayable power level.
 * The vertical axis will display values from the ref level to ref level - dynamic range.
 * |default 100.0
 * |units dB
 * |widget DoubleSpinBox(minimum=10, maximum=200, step=10, decimals=1)
 * |preview disable
 * |tab Axis
 *
 * |param averaging[Averaging] Averaging factor for moving average over FFT bins.
 * A factor of 0.0 means no averaging.
 * A factor of 1.0 means max averaging.
 * Increasing the value increases the averaging window.
 * |default 0.0
 * |preview disable
 * |widget DoubleSpinBox(minimum=0.0, maximum=1.0, step=0.05, decimals=3)
 *
 * |param enableXAxis[Enable X-Axis] Show or hide the horizontal axis markers.
 * |option [Show] true
 * |option [Hide] false
 * |default true
 * |preview disable
 * |tab Axis
 *
 * |param enableYAxis[Enable Y-Axis] Show or hide the vertical axis markers.
 * |option [Show] true
 * |option [Hide] false
 * |default true
 * |preview disable
 * |tab Axis
 *
 * |param yAxisTitle[Y-Axis Title] The title of the verical axis.
 * |default "dB"
 * |widget StringEntry()
 * |preview disable
 * |tab Axis
 *
 * |param freqLabelId[Freq Label ID] Labels with this ID can be used to set the center frequency.
 * To ignore frequency labels, set this parameter to an empty string.
 * |default "rxFreq"
 * |widget StringEntry()
 * |preview disable
 * |tab Labels
 *
 * |param rateLabelId[Rate Label ID] Labels with this ID can be used to set the sample rate.
 * To ignore sample rate labels, set this parameter to an empty string.
 * |default "rxRate"
 * |widget StringEntry()
 * |preview disable
 * |tab Labels
 *
 * |param startLabelId[Start Label ID] Align captured input to the specified label ID.
 * An empty label ID disables this feature.
 * |default ""
 * |widget StringEntry()
 * |preview disable
 * |tab Labels
 *
 * |mode graphWidget
 * |factory /plotters/periodogram(remoteEnv)
 * |initializer setNumInputs(numInputs)
 * |setter setTitle(title)
 * |setter setDisplayRate(displayRate)
 * |setter setSampleRate(sampleRate)
 * |setter setCenterFrequency(centerFreq)
 * |setter setNumFFTBins(numBins)
 * |setter setWindowType(window, windowArgs)
 * |setter setAutoScale(autoScale)
 * |setter setReferenceLevel(refLevel)
 * |setter setDynamicRange(dynRange)
 * |setter setAverageFactor(averaging)
 * |setter enableXAxis(enableXAxis)
 * |setter enableYAxis(enableYAxis)
 * |setter setYAxisTitle(yAxisTitle)
 * |setter setFreqLabelId(freqLabelId)
 * |setter setRateLabelId(rateLabelId)
 * |setter setStartLabelId(startLabelId)
 **********************************************************************/
class Periodogram : public Pothos::Topology
{
public:
    static Topology *make(const Pothos::ProxyEnvironment::Sptr &remoteEnv)
    {
        return new Periodogram(remoteEnv);
    }

    Periodogram(const Pothos::ProxyEnvironment::Sptr &remoteEnv)
    {
        _display.reset(new PeriodogramDisplay());
        _display->setName("Display");

        auto registry = remoteEnv->findProxy("Pothos/BlockRegistry");
        _trigger = registry.callProxy("/comms/wave_trigger");
        _trigger.callVoid("setName", "Trigger");
        _trigger.callVoid("setMode", "PERIODIC");

        //register calls in this topology
        this->registerCall(this, POTHOS_FCN_TUPLE(Periodogram, setNumInputs));
        this->registerCall(this, POTHOS_FCN_TUPLE(Periodogram, setDisplayRate));
        this->registerCall(this, POTHOS_FCN_TUPLE(Periodogram, setNumFFTBins));
        this->registerCall(this, POTHOS_FCN_TUPLE(Periodogram, setFreqLabelId));
        this->registerCall(this, POTHOS_FCN_TUPLE(Periodogram, setRateLabelId));
        this->registerCall(this, POTHOS_FCN_TUPLE(Periodogram, setStartLabelId));

        //connect to internal display block
        this->connect(this, "setTitle", _display, "setTitle");
        this->connect(this, "setSampleRate", _display, "setSampleRate");
        this->connect(this, "setCenterFrequency", _display, "setCenterFrequency");
        this->connect(this, "setNumFFTBins", _display, "setNumFFTBins");
        this->connect(this, "setWindowType", _display, "setWindowType");
        this->connect(this, "setReferenceLevel", _display, "setReferenceLevel");
        this->connect(this, "setDynamicRange", _display, "setDynamicRange");
        this->connect(this, "setAutoScale", _display, "setAutoScale");
        this->connect(this, "setAverageFactor", _display, "setAverageFactor");
        this->connect(this, "enableXAxis", _display, "enableXAxis");
        this->connect(this, "enableYAxis", _display, "enableYAxis");
        this->connect(this, "setYAxisTitle", _display, "setYAxisTitle");
        this->connect(_display, "frequencySelected", this, "frequencySelected");

        //connect to the internal snooper block
        this->connect(this, "setDisplayRate", _trigger, "setEventRate");
        this->connect(this, "setNumFFTBins", _trigger, "setNumPoints");

        //connect stream ports
        this->connect(_trigger, 0, _display, 0);

    }

    Pothos::Object opaqueCallMethod(const std::string &name, const Pothos::Object *inputArgs, const size_t numArgs) const
    {
        //calls that go to the topology
        try
        {
            return Pothos::Topology::opaqueCallMethod(name, inputArgs, numArgs);
        }
        catch (const Pothos::BlockCallNotFound &){}

        //forward everything else to display
        return _display->opaqueCallMethod(name, inputArgs, numArgs);
    }

    void setNumInputs(const size_t numInputs)
    {
        _trigger.callVoid("setNumPorts", numInputs);
        for (size_t i = 0; i < numInputs; i++)
        {
            this->connect(this, i, _trigger, i);
        }
    }

    void setDisplayRate(const double rate)
    {
        _trigger.callVoid("setEventRate", rate);
    }

    void setNumFFTBins(const size_t num)
    {
        _trigger.callVoid("setNumPoints", num);
        _display->setNumFFTBins(num);
    }

    void setFreqLabelId(const std::string &id)
    {
        _display->setFreqLabelId(id);
        _freqLabelId = id;
        this->updateIdsList();
    }

    void setRateLabelId(const std::string &id)
    {
        _display->setRateLabelId(id);
        _rateLabelId = id;
        this->updateIdsList();
    }

    void setStartLabelId(const std::string &id)
    {
        _trigger.callVoid("setLabelId", id);
        _trigger.callVoid("setMode", id.empty()?"PERIODIC":"NORMAL");
    }

    void updateIdsList(void)
    {
        std::vector<std::string> ids;
        if (not _freqLabelId.empty()) ids.push_back(_freqLabelId);
        if (not _rateLabelId.empty()) ids.push_back(_rateLabelId);
        _trigger.callVoid("setIdsList", ids);
    }

private:
    Pothos::Proxy _trigger;
    std::shared_ptr<PeriodogramDisplay> _display;
    std::string _freqLabelId, _rateLabelId;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::BlockRegistry registerPeriodogram(
    "/plotters/periodogram", &Periodogram::make);

static Pothos::BlockRegistry registerPeriodogramOldPath(
    "/widgets/periodogram", &Periodogram::make);

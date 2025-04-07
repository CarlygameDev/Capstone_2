#version 430




struct SpectrumParameters {
	float scale;
	float angle;
	float spreadBlend;
	float swell;
	float alpha;
	float peakOmega;
	float gamma;
	float shortWavesFade;
};

layout(local_size_x = 16, local_size_y = 16) in;  // Workgroup size
layout(rgba16f, binding = 0) uniform image2DArray Spectrums;
layout(std430, binding = 1) buffer SpectrumsBuffer {
    SpectrumParameters _Spectrums[];
};
uniform int domains[4];  


const float PI = 3.14159265359;
const float _Gravity = 9.81;  // Gravity constant
int _Seed=1;


//other parameters
float _Depth=20;



//Spectrum parameters
const float _LowCutoff=0.0001f;
const float _HighCutoff=9000.0f; 


float Dispersion(float kMag) {
    return sqrt(_Gravity * kMag * tanh(min(kMag * _Depth, 20)));
}
float DispersionDerivative(float kMag) {
    float th = tanh(min(kMag * _Depth, 20));
    float ch = cosh(kMag * _Depth);
    return _Gravity * (_Depth * kMag / ch / ch + th) / Dispersion(kMag) / 2.0f;
}


float hash(uint n) {
    // Hugo Elias-style hash with 40-bit constant emulation
    n = (n << 13U) ^ n;

    // Split the 40-bit constant 0x1376312589 into two parts:
    // HIGH_PART = 0x1376 (upper 16 bits)
    // LOW_PART  = 0x312589 (lower 24 bits)
    const uint HIGH_PART = 0x1376U;
    const uint LOW_PART = 0x312589U;

    // Emulate 40-bit addition using modular arithmetic (32-bit wrap-around)
    n = n * (n * n * 15731U + 0x789221U);
    n = n + LOW_PART;   // Add lower 24 bits
    n = n + (HIGH_PART << 24U); // Add upper 16 bits shifted into higher 32 bits

    // Mask to keep the result within 32 bits
    return float(n & 0x7FFFFFFFU) / float(0x7FFFFFFF);
}





float TMACorrection(float omega) {
	float omegaH = omega * sqrt(_Depth / _Gravity);
	if (omegaH <= 1.0f)
		return 0.5f * omegaH * omegaH;
	if (omegaH < 2.0f)
		return 1.0f - 0.5f * (2.0f - omegaH) * (2.0f - omegaH);

	return 1.0f;
}
float SpreadPower(float omega, float peakOmega) {
	if (omega > peakOmega)
		return 9.77f * pow(abs(omega / peakOmega), -2.5f);
	else
		return 6.97f * pow(abs(omega / peakOmega), 5.0f);
}




vec2 UniformToGaussian(float u1, float u2) {
    float R = sqrt(-2.0f * log(u1));
    float theta = 2.0f * PI * u2;

    return vec2(R * cos(theta), R * sin(theta));
}
float JONSWAP(float omega, SpectrumParameters spectrum) {
	float sigma = (omega <= spectrum.peakOmega) ? 0.07f : 0.09f;

	float r = exp(-(omega - spectrum.peakOmega) * (omega - spectrum.peakOmega) / 2.0f / sigma / sigma / spectrum.peakOmega / spectrum.peakOmega);
	
	float oneOverOmega = 1.0f / omega;
	float peakOmegaOverOmega = spectrum.peakOmega / omega;
	return spectrum.scale * TMACorrection(omega) * spectrum.alpha * _Gravity * _Gravity
		* oneOverOmega * oneOverOmega * oneOverOmega * oneOverOmega * oneOverOmega
		* exp(-1.25f * peakOmegaOverOmega * peakOmegaOverOmega * peakOmegaOverOmega * peakOmegaOverOmega)
		* pow(abs(spectrum.gamma), r);
}

float NormalizationFactor(float s) {
    float s2 = s * s;
    float s3 = s2 * s;
    float s4 = s3 * s;
    if (s < 5) return -0.000564f * s4 + 0.00776f * s3 - 0.044f * s2 + 0.192f * s + 0.163f;
    else return -4.80e-08f * s4 + 1.07e-05f * s3 - 9.53e-04f * s2 + 5.90e-02f * s + 3.93e-01f;
}


float Cosine2s(float theta, float s) {
	return NormalizationFactor(s) * pow(abs(cos(0.5f * theta)), 2.0f * s);
}

float ShortWavesFade(float kLength, SpectrumParameters spectrum) {
	return exp(-spectrum.shortWavesFade * spectrum.shortWavesFade * kLength * kLength);
}

float DirectionSpectrum(float theta, float omega, SpectrumParameters spectrum) {
	float s = SpreadPower(omega, spectrum.peakOmega) + 16 * tanh(min(omega / spectrum.peakOmega, 20)) * spectrum.swell * spectrum.swell;
	return mix(2.0f / 3.1415f * cos(theta) * cos(theta), Cosine2s(theta - spectrum.angle, s), spectrum.spreadBlend);
}

void main() {
    

    ivec2 texSize = imageSize(Spectrums).xy;
    uint _N= texSize.x;
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
   
    uint seed = id.x + _N * id.y + _N;
    seed += _Seed;
 
 for (uint i = 0; i < 4; ++i) {
   float halfN = _N / 2.0f;
   float lengthScales=domains[i];
   float deltaK = 2.0f * PI / lengthScales;
   vec2 K = (id.xy - halfN) * deltaK;
   float kLength = length(K);
   
seed += int(i+ hash(seed) * 10);    // Generate random numbers with safety checks
vec4 uniformRandSamples = vec4(hash(seed), hash(seed * 2), hash(seed * 3), hash(seed * 4));
vec2 gauss1 = UniformToGaussian(uniformRandSamples.x, uniformRandSamples.y);
vec2 gauss2 = UniformToGaussian(uniformRandSamples.z, uniformRandSamples.w);

   
     if (_LowCutoff <= kLength && kLength <= _HighCutoff) {
            float kAngle = atan(K.y, K.x);
            float omega = Dispersion(kLength);

            float dOmegadk = DispersionDerivative(kLength);

            float spectrum = JONSWAP(omega, _Spectrums[i * 2]) * DirectionSpectrum(kAngle, omega, _Spectrums[i * 2]) * ShortWavesFade(kLength, _Spectrums[i* 2]);
            
            if (_Spectrums[i * 2 + 1].scale > 0)
                spectrum += JONSWAP(omega, _Spectrums[i * 2 + 1]) * DirectionSpectrum(kAngle, omega, _Spectrums[i * 2 + 1]) * ShortWavesFade(kLength, _Spectrums[i * 2 + 1]);
            
           vec4 result = vec4(vec2(gauss2.x, gauss1.y) * sqrt(2 * spectrum * abs(dOmegadk) / kLength * deltaK * deltaK), 0.0f, 0.0f);
        //    vec4 result = vec4(vec2(gauss2+ gauss1) * sqrt(2 * spectrum * abs(dOmegadk) / kLength * deltaK * deltaK), 0.0f, 0.0f);
           imageStore(Spectrums, ivec3(id,i), result);

        }
        else {
            imageStore(Spectrums, ivec3(id,i), vec4(0.0));
        }

        }
          
}

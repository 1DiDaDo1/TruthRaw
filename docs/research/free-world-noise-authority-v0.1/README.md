# D.RAW Free-World Noise Authority v0.1

Status: **ARCHITECTURE + PRACTICAL DENOISE CONTRACT — NON-DESTRUCTIVE**

## Goal

Reduce visible sensor noise without erasing real texture, edges, CFA-supported detail, censor information, or provenance.

Noise reduction is an estimator, never a rewrite of sealed Direct-CFA evidence.

## Permanent separation

    sealed Direct CFA                 immutable
        -> calibrated noise model     measurement/model
        -> Scientific Master          scientific estimator + uncertainty
        -> TruthNegative Continuous   evidence-bound continuous state
        -> Deep/Open Free World       richer reconstruction/hypotheses
        -> appearance denoise         optional visible rendering aid
        -> display

No denoised result may be written back as measured CFA.

## Sensor-noise model

Start in source-linear RAW code before tone curves and display encoding.

For an uncensored sample with expected signal mu, use a heteroscedastic model:

    Var[y | mu] = a * max(mu, 0) + b

where:
- a models signal-dependent shot-noise/gain behaviour;
- b models signal-independent read/electronic noise.

A richer admitted calibration may replace this with per-ISO, per-readout-domain, per-CFA-site or heavy-tail models. Model identity and fit uncertainty must be recorded.

CENSORED samples are not ordinary noisy observations. Saturation/clipping is a bound/censor event and must stay in the censor model.

## Detail-preserving rule

Denoising strength is limited by uncertainty and structure support.

For every estimate retain:
- source value / source ancestry;
- estimated latent value;
- residual;
- predicted noise sigma;
- reconstruction uncertainty;
- structure/edge confidence;
- authority;
- contribution count;
- whether temporal/multi-view support exists.

A denoiser must fail conservative around:
- strong edges;
- fine repeating texture;
- unresolved CFA chroma;
- specular highlights;
- censor boundaries;
- occlusion/motion boundaries;
- low-confidence registration.

## Single-frame practical path

Single-frame D.RAW should prefer an uncertainty-aware residual estimator rather than unconditional smoothing.

Conceptual residual gate:

    residual = observation - estimate
    normalized = residual / sigma

Only residual energy statistically compatible with the admitted noise model may be strongly suppressed. Structure-inconsistent or model-outlier residuals are retained or assigned higher uncertainty.

Do not use display-space chroma/luma blur as scientific denoising.

## Multi-frame / stop-motion / burst path

Multiple independently sealed RAW observations can reduce noise more defensibly because independent observations contain new measurements.

Requirements:
1. each frame retains its own SHA-256/provenance;
2. register at CFA/sample or scene/ray level where possible;
3. reject/attenuate inconsistent contributors at motion, occlusion and lighting changes;
4. preserve per-frame censor states;
5. combine using noise/uncertainty-aware weights;
6. record effective contributor count and temporal support;
7. never increase the measured-photosite count of an individual frame.

Controlled stop-motion/motion-control is especially useful because repeated poses can provide stable clean plates, illumination passes and sub-pixel sampling while keeping observations separate.

## Free-World noise versus scene variation

The Free World must not mistake legitimate scene variation for noise.

Examples that require protection:
- film/grain-like physical texture if actually in the photographed subject;
- skin pores, fabric weave, foliage, hair;
- small stars / point lights;
- water droplets and sparkle;
- fine shadow texture;
- sensor-supported colour variation at edges;
- temporal object movement.

Noise confidence and scene-detail confidence are separate quantities.

## Appearance-only cleanup

A user may request a visually cleaner render than the scientific estimator justifies. This is allowed downstream as Appearance/Presentation, provided:
- Scientific Master and TruthNegative remain unchanged;
- the cleanup is identified as appearance-derived;
- it creates no evidence;
- it performs no scientific writeback.

## Validation gates

A candidate denoiser is not promoted by PSNR alone.

Validate on:
- flat-field residual statistics;
- dark frames;
- edge MTF/SFR before/after;
- fine texture charts and natural texture;
- colour edges/CFA zippering;
- clipped highlights and censor boundaries;
- low-light colour;
- moving/occluded burst regions;
- difference/residual maps.

Promotion requires no systematic edge-width growth, no texture hallucination, no false colour cleanup that upgrades UNKNOWN/CENSORED information, and residuals compatible with the admitted noise model.

## Initial implementation recommendation

Phase N1 — measurement only:
- estimate/store per-domain shot/read noise parameters and uncertainty;
- expose sigma/variance to Open Scene / TruthNegative;
- do not alter values.

Phase N2 — conservative single-frame estimator:
- uncertainty-normalized residual suppression;
- edge/texture/censor guard;
- keep original and residual lineage.

Phase N3 — temporal evidence fusion:
- use separately sealed frames;
- registration confidence and occlusion masks;
- robust variance-weighted fusion;
- keep frame contribution ancestry.

Phase N4 — appearance cleanup:
- optional user-facing clean render;
- never changes scientific authority.

## Non-claims

- noise cannot be perfectly separated from unknown fine scene detail from one sample;
- a denoised pixel is not newly measured;
- multi-frame fusion is not valid where registration/scene consistency fails;
- a generic Gaussian model is not automatically sufficient for real sensor tails;
- neural denoisers may be useful as estimators but cannot silently manufacture scientific evidence.

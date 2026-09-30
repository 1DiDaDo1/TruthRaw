# D.RAW / TruthRaw — definitieve technische overdracht na cable-audit closure
**Datum:** 30 september 2026  
**Actieve branch:** `research/free-world-radiometric-noise-transport-v01-2026-09-30`  
**Laatste Android-code die groen is gebouwd:** `c90ff62323a065eac54eee0d91f75256708cd4fd`  
**Huidige documentatie-head bij opstellen van dit document:** `db79423cf3636f2c3515ed78cc744b56695a08ab`

## 1. Status in één zin

De implementatie-, UX- en cable-auditronde is afgerond op code-niveau, de Android arm64 debug APK bouwt groen, de belangrijkste provenance/authority/kandidaat-kabels zijn gesloten, maar de nieuwe radiometrie/noise/optics/kleur/tijd/3D/world-space kandidaten zijn nog **niet fysiek gevalideerd en niet wetenschappelijk gepromoveerd**.

## 2. Buildbewijs van de huidige code

De laatste build-proven Android-code is:

- code head: `c90ff62323a065eac54eee0d91f75256708cd4fd`
- workflow: `D.RAW Free World Research APK`
- workflow run: `36746687740`
- conclusie: **SUCCESS**
- artifact: `DRAW-free-world-research-debug-arm64`
- artifact id: `11113326128`
- APK-grootte: **8.155.983 bytes**
- APK SHA-256: `1c5a97b6c5bce7a7534cda9899f1b3d926d9fced8fd8edc16cd53944a6620c03`

Dit bewijst integratie, compilatie en package-vorming. Het bewijst niet dat de nieuwe fysieke modellen op een toestel wetenschappelijk correct zijn.

## 3. Wat het project nu fundamenteel is

D.RAW behandelt een RAW niet als een plaatje dat meteen cosmetisch moet worden verbeterd, maar als een verzegelde observatie waaruit een rijkere fotografische wereld kan worden opgebouwd zonder de bronclaim te overschrijden.

De kern blijft:

```
verzegelde bron/evidence
  -> Universal Intake
  -> Scientific Master
  -> Open Scene / D.RAWnegative / Free World
  -> PURE / ADVANCED / PRO
  -> appearance/export
```

Daarbij blijven de volgende wetten permanent geldig:

- `MEASURED != RECONSTRUCTED != APPEARANCE`
- `BLACK_LEVEL != ZERO_LINE`
- output-rasterdichtheid is niet hetzelfde als optische resolutie
- camera/lens/vendor/RAW/container-identiteit mag decoder-routing beïnvloeden, maar niet de wetenschappelijke waarheid selecteren
- een onbekend residu is niet automatisch noise
- kandidaatbeschikbaarheid is niet hetzelfde als wetenschappelijke promotion
- exacte gemeten CFA-ankers mogen niet stil worden overschreven
- representatie mag de bron overstijgen; kennisclaims niet
- perceptuele denoise blijft appearance-only tenzij een aparte scientific gate ooit aantoonbaar wordt geopend

## 4. Belangrijkste auditbevindingen en wat eraan is gerepareerd

### 4.1 Camera RAW_SENSOR -> DNG lineage

**Auditbevinding:**  
De fysieke camera route verzegelde de RAW_SENSOR-bron correct en gaf de upstream SHA door aan `RawHandle`, maar `UniversalSourceProfile` en de Free World lineage maakten de volledige ancestry niet overal machine-readable zichtbaar.

**Fix:**  
De keten is nu expliciet vastgelegd als:

```
sealed RAW_SENSOR physical evidence root
  -> acquisition evidence
  -> derived processing DNG root
  -> Universal Source Profile
  -> Scientific Master
```

De DNG blijft een nuttige, opnieuw verzegelde processing source, maar wordt niet als een tweede fysieke opname geteld.

`FreeWorldEvidenceLineageManifestV01` onderscheidt nu:
- processing-source roots;
- physical-evidence roots;
- processing -> physical derivations.

### 4.2 Canonical Calibration Observation Record identity

**Auditbevinding:**  
Deduplicatie gebeurde via `JSONObject.toString()`. Dezelfde inhoud met andere JSON-keyvolgorde kon daarom als twee records tellen.

**Fix:**  
`CalibrationObservationRecordIdentityV01` maakt een canonical record-identiteit:
- object-keyvolgorde verandert de identiteit niet;
- arrayvolgorde blijft betekenisvol;
- source SHA-256-roots worden genormaliseerd;
- session-binding metadata bepaalt niet de fysieke record-identiteit;
- echte payloadwijziging levert een andere hash op.

De Android UI dedupliceert nu op die canonical SHA-256 identiteit.

### 4.3 SHA-normalisatie

**Auditbevinding:**  
Een uppercase SHA-256 was syntactisch geldig, terwijl enkele solvers een exacte lowercase lookup tegen een profiel deden.

**Fix:**  
Recordroots en source aliases worden genormaliseerd. De fysieke RAW_SENSOR-root en bewezen afgeleide DNG-root kunnen als alias van dezelfde observatie worden herkend zonder twee onafhankelijke observations te creëren.

### 4.4 Active-session binding

**Auditbevinding:**  
Een geldig Calibration Observation Record uit dataset A kon vóór promotion nog candidate-resultaten in dataset B voeden, omdat record-SHA’s niet verplicht aan de actieve geselecteerde profiles werden gebonden.

**Fix:**  
`CalibrationObservationSessionBindingV01` filtert records voordat een numeric solver ze ziet.

Fail-closed regels:
- iedere recordroot moet bij de actieve observatieset horen;
- fysieke RAW_SENSOR-root en bewezen processing-DNG-alias tellen als dezelfde observation;
- physical + derived alias mogen niet als twee onafhankelijke observations worden gebruikt;
- cross-session recordgebruik is geblokkeerd.

### 4.5 Sterkere recordvalidatie

**Auditbevinding:**  
`validation_status`, uncertainty en observation roles waren eerder te los.

**Fix:**  
De validator controleert nu onder andere:
- ondersteunde axis;
- geldige/unieke SHA-256 roots;
- één niet-lege observation role per root;
- setup description;
- toegestane relation evidence class;
- niet-leeg uncertainty object;
- finite uncertaintywaarden;
- geen negatieve sigma/variance/bound/RMSE;
- expliciete uncertainty-status wanneer geen numerieke uncertainty beschikbaar is;
- herkende validation-status;
- canonical identity consistency;
- recursively forbidden camera/lens/vendor/format identity keys.

Numeric admission is strenger dan syntactische validiteit. Onder andere `UNVALIDATED`, `CANDIDATE`, `REJECTED` en `FAILED` mogen geen numerieke scientific candidate voeden.

### 4.6 Promotion-state was veilig maar onbereikbaar

**Auditbevinding:**  
De nieuwe scientific-denoise- en andere promotion-consumers stonden terecht dicht, maar er bestond nog geen typed route waarlangs later bewezen held-out validation überhaupt een promotion-state zou kunnen leveren.

**Fix:**  
`ScientificPromotionStateV01` is toegevoegd als enige typed toekomstige promotion-ingang.

Die accepteert alleen een intern validation decision met:
- interne-validator provenance;
- held-out validation pass;
- geen measured-sample mutation;
- geen writeback request;
- source roots gebonden aan de actieve sessie.

User-imported JSON kan deze state niet creëren.

De huidige Research Foundation projecteert promotion nog steeds niet.

### 4.7 Promotion firewall uitgebreid

**Auditbevinding:**  
Nieuwe candidate-modules hadden extra side-effectvelden die niet allemaal onder de generieke recursive firewall vielen.

**Fix:**  
De firewall blokkeert nu ook true voor:
- `candidate_applied`
- `scientific_master_modified`
- `measured_anchor_modified`
- `measured_anchors_modified`
- `image_transform_applied`

Naast de bestaande promotion/correction/deconvolution/denoise/writeback-flags.

### 4.8 Kleurmatrix en uncertainty/covariance

**Auditbevinding:**  
De 3x3 kleurfit bestond en `ScientificNoiseMathV01` kon `C_out = J*C_in*J^T` uitvoeren, maar beide uiteinden waren nog niet end-to-end verbonden.

**Fix:**  
`ColourCovarianceTransportCandidateV01` koppelt de kleurrelatie aan covariance transport.

Belangrijk:
- alleen expliciet aangeleverde camera-RGB covariance wordt gebruikt;
- ontbrekende covariance wordt niet verzonnen;
- er wordt niets toegepast op Scientific Master;
- output blijft candidate/unpromoted.

### 4.9 NPS + optica + inverse optics

**Auditbevinding:**  
NPS, SFR/MTF/PSF en de noise-aware inverse-optics operator bestonden afzonderlijk, maar er was nog geen end-to-end bridge.

**Fix:**  
`NoiseOpticsJointCandidateV01` kan nu combineren:
- controlled NPS;
- controlled MTF/SFR;
- expliciete signal PSD.

Daaruit kan de bestaande Wiener-achtige gain candidate worden gevoed.

Niet toegestaan:
- signal PSD uit de foto verzinnen;
- MTF uit "ziet er scherp uit" afleiden;
- image transform toepassen;
- deconvolution autoriseren.

### 4.10 Geometry/world-space authority

**Auditbevinding:**  
Imported relation-records konden velden bevatten zoals `pose_relation_admitted`, `world_to_source_relation_admitted` of `radiometric_relation_admitted`. Zulke velden mogen nooit op zichzelf promotion-authority worden.

**Fix:**  
Nested samples/rays worden aan admitted session roots gebonden. Promotion-consumers gebruiken de typed internal promotion state en de gevalideerde world->source bridge. Imported booleans kunnen candidate-evidence structureren maar geen promotion verlenen.

### 4.11 World->source bridge

De future bridge vereist nu expliciet:
- promoted world-to-source state;
- een exact gevalideerde mapping;
- held-out validation;
- mapping uncertainty;
- non-appearance authority;
- source roots gebonden aan de actieve sessie.

De huidige standaard blijft fail-closed.

### 4.12 Reconstruction/denoise

`ScientificReconstructionCandidateV01` bewaart exacte MEASURED anchors.

Niet-anchors kunnen alleen reconstructed candidate worden.

`ScientificDenoiseOperatorV01` heeft nu een toekomstig pad via `ScientificPromotionStateV01`, maar de huidige Research Foundation geeft `allowPromotionProjection=false`.

Zelfs een toekomstige admitted derived route:
- maakt geen nieuwe physical evidence;
- zet reconstructed niet om naar measured;
- wijzigt Scientific Master niet automatisch;
- geeft geen writeback zonder aparte gate.

### 4.13 Research navigation/picker state

**Auditbevindingen:**
- Research mode kon na een hergebruikte Activity onbedoeld blijven hangen;
- "Open RAW/DNG direct" opende niet altijd opnieuw de picker wanneer al een session bestond;
- grote relation-records waren activity-memory gevoelig.

**Fixes:**
- normale intent reset Research mode;
- direct RAW/DNG kan de picker opnieuw openen met bestaande sessie;
- relation-records gebruiken een private session cache via `CalibrationObservationRecordSessionStoreV01`;
- die cache is geen evidence en wordt bij doelbewust beëindigen opgeruimd.

### 4.14 Groen/rood punt + timer

De moderne werkbank had het principe al.

`TruthRawLegacyTestStatusV01` past dezelfde conventie nu ook toe op historische diagnostics/oracles vanuit Settings:

- groen + live `Looptijd` tijdens running;
- groen + `Gereed in` bij succes;
- rood + `Gestopt na` bij error/fail-closed;
- muted idle.

Dit verandert nooit scientific authority.

## 5. Wat tijdens de audit juist goed bleek aangesloten

### 5.1 Float64 -> Float32

De branch-sensitive reconstruction/decision-kant gebruikt Float64 waar die precisie nodig is.

De Scientific Master / wetenschappelijke storage/export gebruikt gecontroleerde Float32 waar het contract dat toelaat.

Meer bits worden niet als meer evidence behandeld.

### 5.2 Zero-Line / BlackLevel

BlackLevel blijft bron-/codingcontext.

Zero-Line is een downstream scientific representatieconcept en wordt niet stil gelijkgesteld aan BlackLevel.

### 5.3 Frontside / backside

Backside:
- source bytes;
- CFA;
- metadata;
- measured support;
- sparse exact sample grid.

Frontside:
- deterministische zichtbare structuur;
- edges/proporties;
- appearance-derived protection/context.

Frontside mag backside CFA evidence niet herschrijven.

### 5.4 D.RAWnegative / Open Scene

D.RAWnegative blijft een rijkere observation-bound state boven de Scientific Master. De nieuwe radiometrie/noise/3D/optics modules vervangen D.RAWnegative niet; ze kunnen er later meer onderbouwde kennis aan leveren.

### 5.5 PURE / ADVANCED / PRO

Alle routes delen dezelfde sealed source en Scientific Master.

- PURE = Scientific View
- ADVANCED = downstream Appearance / Restoration View
- PRO = Open Scene / provenance / professionele en research workbench

PRO heeft meer gereedschap, niet meer evidence.

### 5.6 Restoration

Restoration blijft derived/retreatable. Ontbrekende of beschadigde delen worden niet MEASURED omdat ze gereconstrueerd zijn.

### 5.7 JPEG-preview

JPEG-preview is non-authority preview-only.

Ook de speciale 200MP-uitgang gebruikt geen JPEG-overlay als scientific primary.

### 5.8 4K / Camera-5 -> 200MP

Deze route blijft apart.

Een dense 200MP representatie is reconstruction/support, geen claim dat 200MP onafhankelijke CFA-metingen bestaan.

### 5.9 Compute

De generieke compute-router blijft bewust `CPU_REFERENCE` totdat een individuele accelerated kernel zijn eigen equivalence/correctness/self-test heeft.

Bestaande gespecialiseerde routes, zoals TruthNegative Vulkan en restoration multicore, blijven afzonderlijk.

Snelheid mag nooit authority verhogen.

## 6. Nieuwe praktische appstructuur

De app is nu logisch gesplitst.

### Beginpagina

Eerst:
- actieve route;
- Bestand;
- Universele camera.

Daarna routekeuze:
- PURE;
- ADVANCED;
- PRO.

Los daarvan:
- Research & JSON;
- Wat kan D.RAW nu?

### Research & JSON

Centrale ingang voor:
- Multi-observation;
- Calibration Observation Records;
- field/atlas exports;
- Free World Foundation;
- Global Research Snapshot;
- kandidaatdiagnostiek.

Normale RAW-verwerking vereist deze laag niet.

### Global Research Snapshot

De Global Research Snapshot beschrijft de projectarchitectuur, gates en implementatiestatus.

Hij bevat geen fotometing en is geen calibration evidence.

### Teststatus

Het groene/rode punt + timerprincipe blijft standaard voor research/tests.

## 7. Intentional boundaries — bewust zo laten

De volgende punten zijn **geen losse kabels**.

### 7.1 Proprietary RAW decode coverage

Wetenschappelijk is de architectuur vendor/lens/RAW-identity onafhankelijk.

Decodertechnisch is niet ieder proprietary RAW-formaat al volledig scientific-processing-ready.

Huidige grens:
- DNG = native admitted processing path;
- NEF = beperkte/sample decoder, calibration pending;
- CR3/CR2/ARW/RAF/RW2/ORF/PEF/IIQ/X3F e.a. = sealed ingress mogelijk, decoder pending.

Niet doen: een onbekende proprietary RAW stil als DNG of gemeten CFA behandelen.

### 7.2 Scientific Master Streaming v0.3

v0.3 blijft research candidate.

Production/native bridges blijven op de huidige admitted binding totdat v0.3 afzonderlijk bewezen wordt.

### 7.3 Continuous Free World Query

Blijft UNKNOWN totdat world->source mapping en continuous reconstruction fysiek gevalideerd zijn.

### 7.4 Geen AI/ML

De huidige wetenschappelijke kern gebruikt geen neural/generative model als source of truth.

### 7.5 Build warning

De groene Android build rapporteert een bestaande C++ compiler-warning in:

`canonical/reconstruction/v4.7i/native/src/core.cpp`

rond een compacte `}return Status::ok();` constructie / misleading indentation.

De build slaagt. Deze audit heeft de canonical reconstruction bewust niet alleen voor een formatteringswarning gewijzigd.

## 8. Wat jij nu nog moet uitvoeren

Dit is de praktische volgende fase. Niet alles hoeft in één dag. Normaal fotograferen kan al zonder deze calibratiecampagne.

### Fase A — APK smoke test

Gebruik de APK met SHA-256:

`1c5a97b6c5bce7a7534cda9899f1b3d926d9fced8fd8edc16cd53944a6620c03`

Controleer op toestel:

1. beginpagina opent;
2. PURE / ADVANCED / PRO kunnen worden gekozen;
3. Bestand opent ook opnieuw wanneer al een RAW geladen is;
4. Universele camera opent;
5. Research & JSON opent;
6. "Wat kan D.RAW nu?" opent;
7. Multi-observation Research-modus kan geopend en gesloten worden;
8. groen/rood punt + timer verschijnt bij test/analysis;
9. een normale bekende DNG verwerkt nog steeds via de bestaande Scientific Master-route;
10. de speciale 200MP-route blijft apart bereikbaar waar hij eerder admitted was.

Doel van deze fase: runtime-regressies vinden, niet wetenschappelijke promotion.

### Fase B — camera lineage controleren

Maak minimaal één normale opname via de Universele camera.

Controleer de geëxporteerde/diagnostische lineage:

```
RAW_SENSOR physical root
-> acquisition evidence
-> derived DNG processing root
-> Universal Source Profile
```

De fysieke frame count mag hierdoor niet verdubbelen.

Herhaal dit later voor ultra-wide, wide/main en tele, maar gebruik lensnamen alleen als acquisitie/provenance, niet als scientific calibration key.

### Fase C — natural overlap same route

Minimaal:
- 3 onafhankelijk verzegelde overlappende RAW-observaties;
- betekenisvolle sensor/camerapositie-verandering;
- 1 held-out observation.

Doel:
- deterministic features;
- pair geometry;
- tracks;
- loop consistency.

Nog geen automatische world-registration promotion.

### Fase D — cross optical route overlap

Minimaal:
- 3 overlappende sealed RAW observations uit verschillende optical routes, bijvoorbeeld UW/main/tele;
- camera/lensnaam niet als relation key.

Doel:
bewijzen dat wereldstructuur gedeeld kan worden zonder calibratie tussen de routes gelijk te stellen.

### Fase E — rotation / field separation

Voor de gecontroleerde variant bij voorkeur:
- 0 graden;
- 90 graden;
- 180 graden;
- 270 graden;

van een voldoende uniform veld, met setup uncertainty genoteerd.

Doel:
world-fixed versus source/sensor-fixed veldgedrag scheiden.

Niet automatisch "lens-only vignetting" noemen.

### Fase F — radiometric response / linearity

Maak een gecontroleerde exposure-serie met:
- constante scene/illumination waar mogelijk;
- bekende exposure context;
- herhaalde levels;
- held-out observations.

Te onderzoeken:
- exposure-to-code response;
- effective gain;
- black/offset;
- saturation/censor behavior;
- linearity.

BlackLevel blijft iets anders dan Zero-Line.

### Fase G — dark/noise en component separation

Nodig:
- meerdere onafhankelijke dark observations;
- meerdere illuminated/signal-level observations;
- exposure/gain context;
- repeated frames;
- held-out validation.

Te scheiden:
- temporal random noise;
- read noise candidate;
- dark-current behavior;
- DSNU;
- PRNU-like fixed response;
- row/column correlation;
- CFA-phase behavior;
- NPS;
- UNKNOWN residual.

Nooit: "alles wat overblijft = noise".

### Fase H — kleur onder meerdere illuminants

Gebruik:
- een bekende reference target;
- minimaal twee gekarakteriseerde illuminants;
- voldoende train patches;
- held-out patches;
- uncertainty.

Doel:
camera-RGB -> reference-XYZ relation candidate plus covariance transport.

RGB blijft geen spectrometer.

### Fase I — optical support

Gebruik gecontroleerde:
- SFR;
- MTF;
- PSF of equivalent;

met:
- field coordinates;
- focus state;
- uncertainty;
- held-out validation.

Pas nadat echte optical support bestaat kan inverse-optics wetenschappelijk worden beoordeeld.

### Fase J — temporal / stop-motion

Gebruik originele onafhankelijke RAW-observaties met:
- source-bound timing;
- sequence membership;
- waar mogelijk readout/rolling-shutter context;
- held-out relation check.

Een synthetisch tussenframe is nooit extra fysieke evidence.

### Fase K — geometry / depth / visibility

Gebruik meerdere overlappende observations met onafhankelijk controleerbare geometry.

2D appearance alignment alleen is onvoldoende.

Doel:
- parallax;
- depth candidate;
- visibility;
- occlusion;
- pose/ray relations.

Geometrische confidence mag radiometric authority niet upgraden.

### Fase L — world-space residual separation

Pas uitvoeren nadat de relevante world->source en radiometric relaties voldoende zijn gevalideerd.

Nodig:
- meerdere onafhankelijke observations;
- sensor-position diversity;
- uncertainty;
- held-out observations.

Doel:
onderscheid onderzoeken tussen:
- world-fixed signal;
- sensor-fixed pattern;
- temporal residual;
- view dependence;
- motion/occlusion;
- UNKNOWN.

### Fase M — axis-specifieke promotion

Promotion mag pas worden overwogen wanneer de relevante gate zijn eigen bewijs heeft.

Niet in één keer "alles groen" maken.

Voor iedere axis afzonderlijk:
1. gecontroleerde physical evidence;
2. held-out validation;
3. uncertainty;
4. source binding;
5. geen measured mutation;
6. intern promotion decision;
7. daarna pas eventueel een downstream operator toelaten.

### Fase N — scientific denoise als laatste

Pas nadat onder andere:
- radiometric calibration promoted;
- noise-component calibration promoted;
- numeric noise transport validated;

kan single-frame scientific denoise überhaupt evidence-ready worden.

Optics-aware vraagt bovendien promoted optical support.

World-space denoise vraagt bovendien:
- validated world->source;
- temporal relation;
- geometry/depth waar nodig;
- world-space noise separation.

Ook dan blijft Scientific Master-writeback een aparte gate.

## 9. Wat jij niet hoeft te doen

Voor normaal gebruik hoef je niet:

- voor ieder toestel handmatig een lensprofiel te maken;
- camera-ID's in een scientific model te programmeren;
- voor iedere foto een Calibration Observation Record te importeren;
- de research-campagne te draaien voordat je een normale DNG opent;
- een panorama te maken om individuele RAW observations evidence te laten zijn;
- een AI-model te trainen;
- een onbekend residual als noise te labelen.

## 10. Aanbevolen testvolgorde

De meest efficiënte volgorde is:

1. smoke test APK;
2. bekende goede DNG;
3. Universele camera lineage;
4. 3x same-route overlap;
5. controlled rotation;
6. radiometric exposure-serie;
7. dark/flat/repeated noise-serie;
8. colour target multi-illuminant;
9. optical target;
10. temporal/stop-motion;
11. geometry/world-space;
12. pas daarna promotiononderzoek.

Deze volgorde maximaliseert vroege feedback zonder dat een latere complexe test een eenvoudige UI/lineage-regressie maskeert.

## 11. Belangrijkste historische wetenschappelijke ankerpunten die behouden moeten blijven

### PR90 anchor-constrained reconstruction

De eerdere real-device hold-out liet zien dat meer dekking niet automatisch betere reconstructie betekent.

Belangrijkste les:
- affine had meer geldige punten;
- gemiddelde fout was slechter dan baseline op de direct vergelijkbare punten;
- uncertainty was te optimistisch;
- solver werd terecht niet gepromoveerd.

Die les blijft centraal: **meer modelcomplexiteit of dekking is geen reden voor promotion zonder held-out winst en gekalibreerde uncertainty.**

### Field-repeatability

Eerdere field-resultaten lieten zien dat gemeten veldvormen niet simpel als lens-only vignetting mogen worden geïnterpreteerd.

Daarom blijft controlled rotation/world-vs-sensor separation noodzakelijk.

### Camera-5 / 200MP

Een capability/envelope is geen bewijs van 200MP onafhankelijke sensorinformatie.

Daarom blijft 4K/Camera-5 -> 200MP een reconstructed outputroute met expliciete authority.

## 12. Huidige eindconclusie

De code heeft nu geen bekende grote "losse kabel" meer uit de auditlijst die door simpelweg extra wiring kan worden opgelost zonder nieuwe fysieke informatie.

Wat nog openstaat valt in drie groepen:

1. **device/runtime validation** — werkt de groene build op het toestel zoals bedoeld?
2. **physical scientific validation** — kloppen de candidate-modellen onder gecontroleerde held-out experimenten?
3. **decoder/accelerator uitbreiding** — extra proprietary RAW-decoders en algemene acceleratie zijn aparte toekomstige engineeringprojecten.

De juiste volgende stap is dus niet opnieuw architectuur toevoegen, maar de bestaande architectuur **met echte observaties onder druk zetten** en uitsluitend die assen promoveren die hun eigen evidence-gate halen.

## 13. Canonieke bestanden om voortaan eerst te lezen

1. `docs/research/free-world-radiometric-noise-transport-v0.1/FINAL_PROJECT_HANDOFF_2026-09-30.md`
2. `docs/research/free-world-radiometric-noise-transport-v0.1/CABLE_AUDIT_CLOSURE_2026-09-30.md`
3. `state/FREE_WORLD_RADIOMETRIC_NOISE_TRANSPORT_STATE_2026-09-30.json`
4. `docs/research/free-world-radiometric-noise-transport-v0.1/README.md`
5. `docs/research/free-world-radiometric-noise-transport-v0.1/UX_NAVIGATION_2026-09-30.md`
6. daarna pas historische PR101/PR100/PR99 documenten indien nodig.

## 14. Bevroren eindstatus voor overdracht

- code implementation: compleet voor de huidige cable-audit recommendation wave;
- Android compile/build: groen op `c90ff62323a065eac54eee0d91f75256708cd4fd`;
- APK geverifieerd;
- device validation: nog uitvoeren;
- physical validation campaign: nog uitvoeren;
- scientific promotion: geen;
- Scientific Master writeback vanuit research-candidates: verboden;
- normal user calibration: niet vereist;
- volgende fase: **device + physical validation, daarna uitsluitend axis-specifieke promotion**.

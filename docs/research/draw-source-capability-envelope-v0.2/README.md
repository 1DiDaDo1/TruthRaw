# D.RAW Source Capability Envelope v0.2 — Physical Knowledge Map

Status: **CURRENT PHYSICAL SOURCE CAPABILITY ENVELOPE — HOST VALIDATED**

v0.2 binds capabilities to the stable physical Observation Record v0.4 rather
than to a compatibility ingress container.

It also fixes the source-domain limitation exposed by the real main source:
serialized storage can be known while capture-sample domain, readout domain
and sensor pixel mode remain UNKNOWN.

## Domain separation

v0.2 separately stores:

- physical Camera2 capture route;
- serialized DNG/CFA storage domain;
- derived compatibility ingress lineage;
- capture sample domain;
- readout domain;
- sensor pixel mode.

The last three are currently UNKNOWN for main.

## Capability origin

Every capability now records where its knowledge comes from:

- `PHYSICAL_SOURCE`;
- `PIPELINE_SCIENTIFIC_DERIVED`;
- `SOURCE_METADATA_LINEAGE_BOUND`;
- `PHYSICAL_SOURCE_AND_PIPELINE_LINEAGE`;
- `UNKNOWN`.

Unknown capability never receives an invented identity or lineage hash.

## Main state

Physical Observation:

`DRAW_PHYSICAL_OBS_a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`

Physical graph node:

`029b845daf0bb20ebed4e7ce6d66eb61a172dd89e9977f996d20d604c2703353`

Capability Envelope v0.2 state:

`057c627bf172521d9336386d4f74ae1e26664c70c4a24ed7993b6c16c33a3986`

No calibration, graph relation or fusion authority is created by this
envelope.

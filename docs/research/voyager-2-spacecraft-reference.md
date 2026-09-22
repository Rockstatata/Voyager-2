# Voyager 2 spacecraft geometry reference

## Scope

This note is a procedural-model reference for Voyager 2. It separates published dimensions and hardware relationships from implementation choices. NASA describes Voyager 1 and 2 as identical spacecraft, so authoritative drawings and models of either twin are useful here. The NASA 3D assets are references only; the project should build its own geometry rather than import their meshes.

## Sourced facts

### Scale anchors

| Structure | Published geometry | Procedural-model consequence |
| --- | --- | --- |
| High-gain antenna (HGA) | 3.7 m diameter in NASA's FAQ; JPL gives 3.66 m for the main reflector. [S1][S4] | Use the dish as the scale anchor and largest compact form. |
| Electronics bus | Ten-sided hollow compartment, 74 in (1.88 m) diameter and 18.5 in (0.47 m) high in the NASA flight-data report; NASA's public FAQ rounds its diameter to about 1.8 m. [S1][S2] | The dish diameter is about 1.95 times the bus diameter. A bus wider than the dish reverses the real silhouette. Model ten distinct side bays. |
| Overall compact height | About 3.8 m from the top of the reflector structure to the triangular feet below the bus. [S1] | Use this only as a whole-spacecraft check; it does not include deployed booms. |
| Magnetometer boom | 13 m deployed. It telescoped and rotated out of a roughly 2 ft canister. [S1][S15] | It must be the dominant long, lightweight truss/rod, not a thick solid arm. |
| RTG boom | 3.7 m. Three RTGs are mounted tandem, end-to-end, on the deployable boom. [S1][S3] | Include a real support boom before the generator chain; do not attach three plain cylinders directly to the bus. |
| Science-instrument boom | 3 m. [S1] | Use 3.0 m, not 2.5 m. Its outboard end carries the scan platform. |
| PRA/PWS antennas | Two extendible elements, 10 m each, arranged at right angles and read together as a wide V. Each element is about 0.5 in (12.7 mm) diameter. [S1][S7] | Render as extremely thin rods/tubes. Their length and V silhouette matter more than visible thickness. |
| Individual MHW-RTG | About 0.58 m long and 0.40 m across the fin tips, with six radial heat-rejection fins. [S13] | Each unit should be a ribbed/finned cylinder; three repeat along one axis. |

### Body axis, antenna stack, and bus

- The bus is a decagonal prism with one electronics bay on each side. Its centerline is the spacecraft roll/z axis. The HGA is fixed to this axis and points toward Earth; the PDS convention defines the HGA boresight as **-Z**. [S3][S6]
- The HGA is a circular parabolic reflector, not a flat disc or a cone with a closed face. JPL's axial antenna stack runs outward from the dish through the X-band feed, frequency-selective subreflector, S-band feed, and finally the low-gain antenna (LGA). The LGA is mounted on the back of the S-band feed structure. [S4]
- The visible center assembly therefore needs a feed/subreflector body held in front of the dish by thin struts, plus the small LGA at the outer tip. A dish alone misses one of Voyager's strongest recognition cues. This last sentence is an implementation inference from the sourced stack.
- Small attitude-control thrusters are attached around the bus. Voyager has 16 hydrazine thrusters used for attitude control and trajectory correction, with redundant groupings; it does not have one visually dominant main engine. [S5][S6][S8]
- The optical calibration target is a fixed, flat rectangle of known color and brightness which the scan-platform instruments can view. [S3]

### Boom and instrument relationships

- The **science boom and RTG boom are mounted on opposite sides of the bus**, a layout that helps shield science instruments from radiation from the power source. [S2]
- The science boom carries the plasma, cosmic-ray, and low-energy charged-particle instruments inboard. At its outboard end, a two-degree-of-freedom scan platform carries the wide- and narrow-angle cameras, ultraviolet spectrometer, infrared interferometer spectrometer/radiometer, photopolarimeter, and imaging control electronics. [S5][S6]
- The magnetometer system uses the separate 13 m boom. The detailed PDS host record places two low-field magnetometers along its outer portion (one at the tip and one about 3 m inboard) and two high-field magnetometers near the boom base. [S6]
- Three RTGs form one continuous end-to-end chain on their own boom. Each real unit has conspicuous radial fins; the repeated fin stacks are more recognizable than smooth canisters. [S3][S13]
- The two 10 m PRA/PWS elements share a root assembly and form orthogonal monopoles/a balanced V-shaped dipole depending on instrument use. [S3][S7]

## Visual arrangement observed in official references

The following is **reference observation**, not a set of additional engineering measurements. In JPL's labeled three-quarter concept, the dish dominates the center, the science/scan-platform cluster projects to viewer-right, the long magnetometer boom reaches upper-left, the RTG chain sits lower-left, and the two thin PRA/PWS rods open toward the left. [S9] NASA's side-profile image and assembly gallery confirm that the science end is a dense, irregular collection of boxes and camera apertures, while the bus, booms, and antenna supports are open trusses rather than a collection of solid cylinders. [S11][S12]

The two local references show the same recognition cues and were inspected during this research: [front view](../../images/Voyager-2-front.jpg) and [side view](../../images/Voyager-2-side.jpg). Their original provenance was not established here, so they are not used as dimensional evidence.

## Implementation inference: recognizable procedural hierarchy

This is the minimum hierarchy likely to read as Voyager from more than one angle:

```text
Voyager root
|- decagonal bus
|  |- ten bay panels / thermal-blanket blocks
|  |- small thruster nozzles around the rim/back
|  |- golden-record disc on an exposed side
|  `- calibration target / radiator panel
|- HGA paraboloid
|  `- feed-strut assembly -> feeds/subreflector -> small LGA
|- RTG truss boom -> 3 finned RTGs in tandem
|- science truss boom
|  |- inboard particle/plasma instrument boxes
|  `- articulated scan platform -> twin cameras and sensor heads
|- deployable magnetometer boom -> visible sensor packages
`- PRA/PWS root -> two very thin 10 m elements forming a V
```

Use one consistent real-space ratio before applying a separate display scale. Recommended normalized dimensions with HGA diameter = `1.0` are: bus diameter `0.51`, bus height `0.13`, magnetometer boom `3.55`, RTG boom `1.01`, science boom `0.82`, each PRA/PWS element `2.73`, RTG length `0.16`, and RTG fin-tip diameter `0.11`. These ratios are calculations from the sourced measurements, not separately published values.

Preserve the project's chosen local `+Z` flight/heading convention if needed, but treat that as a control convention. The physical antenna axis should be documented independently: NASA/PDS defines the HGA boresight as spacecraft `-Z`, and the real spacecraft's travel direction is not a permanently fixed visual "nose." [S6]

## Camera-worthy views and verification captures

These are implementation recommendations inferred from the official imagery, not prescribed NASA attitudes:

1. **Front three-quarter hero view:** look from the HGA side but offset laterally and vertically. Keep the dish elliptical rather than circular in frame, with the bus rim, feed/LGA, RTGs, and scan platform all visible. Match the broad arrangement in JPL PIA14111. [S9]
2. **Exact side profile:** frame the complete compact spacecraft and the starts of all booms. This best checks dish depth, bus height, science-platform density, RTG spacing, and that the dish is about twice the bus width. Compare with NASA's official side profile. [S12]
3. **Rear/bus three-quarter:** look past the back of the dish toward the bus. This should reveal ten-sided bay construction, Golden Record, calibration/radiator panel, boom roots, and small thruster nozzles.
4. **Wide silhouette view:** pull back far enough to include the full 13 m magnetometer boom and both 10 m V antennas. Thin-line visibility may require a presentation-only minimum radius, but lengths and directions should remain proportionally correct.
5. **Dish-axis technical view:** look along the HGA boresight only as a validation shot. It proves dish/feed/LGA concentricity but is a weak default gameplay camera because the dish hides the bus and boom roots.

## Highest-impact corrections to a simplified model

In priority order:

1. Restore the real dish-to-bus proportion and use a concave/open paraboloid.
2. Add the feed-support structure and low-gain antenna in front of the dish.
3. Separate the three major booms spatially; specifically keep the science and RTG booms on opposite bus sides.
4. Build the 3 m science boom and its crowded scan-platform head, including two obvious camera apertures.
5. Replace smooth RTG cylinders with three six-finned units on a 3.7 m truss.
6. Make the magnetometer boom long and delicate, with sensor packages rather than a thick uniform cylinder.
7. Make the two 10 m antennas an orthogonal V of hair-thin rods.
8. Add secondary silhouette cues: decagonal bays, multiple small thruster nozzles, radiator/calibration panel, and Golden Record.

## Primary sources

- **S1 — NASA Science, Voyager FAQ:** dimensions of the bus, dish, booms, antennas, and overall height. <https://science.nasa.gov/mission/voyager/frequently-asked-questions/>
- **S2 — NASA Glenn, *Titan/Centaur D-1T TC-6 Voyager Flight Data Report* (1976):** exact bus dimensions and opposite science/RTG boom relationship. <https://www1.grc.nasa.gov/wp-content/uploads/TC-6-Voyager-Flight-Data-Report-1976.pdf>
- **S3 — NASA Science, Voyager spacecraft:** decagonal bus, axis relationship, HGA, optical target, RTG arrangement, and PWS/PRA description. <https://science.nasa.gov/mission/voyager/spacecraft/>
- **S4 — JPL DESCANSO, *Voyager Telecommunications*:** spacecraft drawing, 3.66 m reflector, feed stack, and LGA placement; see Figure 1-2 and Section 3.4. <https://descanso.jpl.nasa.gov/DPSummary/Descanso4--Voyager_new.pdf>
- **S5 — NASA Planetary Data System, Voyager mission description:** 16 thrusters and science-boom/scan-platform payload. <https://pds.nasa.gov/data/vg1-j-pos-6-summ-s3coords-v1.1/vg_1501/document/mission/mission.htm>
- **S6 — NASA PDS Ring-Moon Systems Node, Voyager host description:** coordinate axes, boom directions, thruster redundancy, RTGs, scan platform, and magnetometer placement. <https://pds-rings.seti.org/voyager/uvs/vg1host.html>
- **S7 — NASA PDS, Voyager Plasma Wave Investigation:** two orthogonal 10 m antenna elements, diameter, mechanism, and V/dipole use. <https://pds.nasa.gov/data/vg1-j-spice-6-spk-v2.0/vg_1501/document/pws/pwsinst.htm>
- **S8 — JPL Interplanetary Network Progress Report, Figure 7:** labeled spacecraft configuration including four magnetometers, 16 thrusters, science instruments, RTGs, and antennas. <https://ipnpr.jpl.nasa.gov/progress_report2/42-44/44C.PDF>
- **S9 — JPL PIA14111, model of Voyager:** labeled three-quarter component arrangement. <https://www.jpl.nasa.gov/images/pia14111-model-of-voyager-artist-concept/>
- **S10 — NASA VTAD Voyager 3D reference:** official interactive/downloadable visual model; use as a turntable reference, not as project geometry. <https://science.nasa.gov/resource/voyager-3d-model/>
- **S11 — NASA Science, Images of Voyager:** archival assembly and test photography. <https://science.nasa.gov/gallery/images-of-voyager/>
- **S12 — NASA Science, Voyager side profile:** official profile reference. <https://science.nasa.gov/image-detail/spacecraft-profile-3/>
- **S13 — NASA Technical Reports Server, MHW-RTG dimensional reference:** Voyager RTG length, fin-tip diameter, and six-fin construction. <https://ntrs.nasa.gov/api/citations/20160001769/downloads/20160001769.pdf?attachment=true>
- **S14 — NASA Science, Golden Record construction and cover:** 12 in (30 cm) record and spacecraft-mounted visual reference. <https://science.nasa.gov/mission/voyager/making-of-the-golden-record/> and <https://science.nasa.gov/mission/voyager/golden-record-cover/>
- **S15 — NASA Science, Voyager “Did You Know?”:** deployment of the magnetometer boom from its compact launch canister. <https://science.nasa.gov/mission/voyager/did-you-know/>

The NASA 3D model and artist concepts are strong shape/layout references, but they are not engineering CAD. Where a visual asset and a published number differ, use the published dimension and document any deliberate display exaggeration.

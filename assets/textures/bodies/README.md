# Body Textures

26 real, credited image maps — one per registered celestial body, loaded at runtime by `Texture2D::loadFromFile` (`stb_image` JPEG decode, see [docs/objects/phase-2-rendering-pipeline.md](../../../docs/objects/phase-2-rendering-pipeline.md)). Normal and specular maps are derived from these images at load time.

| File | Body | Resolution | Source |
| --- | --- | ---: | --- |
| `sun.jpg` | Sun | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `mercury.jpg` | Mercury | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `venus.jpg` | Venus | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `earth.jpg` | Earth | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `mars.jpg` | Mars | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `jupiter.jpg` | Jupiter | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `saturn.jpg` | Saturn | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `uranus.jpg` | Uranus | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `neptune.jpg` | Neptune | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `moon.jpg` | Moon | 2048x1024 | Solar System Scope, CC BY 4.0 |
| `io.jpg` | Io | 1440x720 | NASA 3D Resources, public domain |
| `europa.jpg` | Europa | 1440x720 | NASA 3D Resources, public domain |
| `ganymede.jpg` | Ganymede | 1440x720 | NASA 3D Resources, public domain |
| `callisto.jpg` | Callisto | 1440x720 | NASA 3D Resources, public domain |
| `titan.jpg` | Titan | 720x360 | NASA 3D Resources, public domain |
| `miranda.jpg` | Miranda | 1440x720 | NASA 3D Resources, public domain |
| `ariel.jpg` | Ariel | 1440x720 | NASA 3D Resources, public domain |
| `umbriel.jpg` | Umbriel | 1440x720 | NASA 3D Resources, public domain |
| `titania.jpg` | Titania | 1440x720 | NASA 3D Resources, public domain |
| `oberon.jpg` | Oberon | 1440x720 | NASA 3D Resources, public domain |
| `triton.jpg` | Triton | 1440x720 | NASA 3D Resources, public domain |
| `pluto.jpg` | Pluto | 720x360 | NASA 3D Resources, public domain |
| `tethys.jpg` | Tethys | 1440x720 | NASA 3D Resources, public domain |
| `dione.jpg` | Dione | 1440x720 | NASA 3D Resources, public domain |
| `rhea.jpg` | Rhea | 1440x720 | NASA 3D Resources, public domain |
| `iapetus.jpg` | Iapetus | 1440x720 | NASA 3D Resources, public domain |

Sources:
- **Solar System Scope** free 2k texture pack — CC BY 4.0 — https://www.solarsystemscope.com/textures/
- **NASA 3D Resources** — public domain ("free and without copyright" per the repository README) — https://github.com/nasa/NASA-3D-Resources

Per-body provenance, geometry, and transform details are in `docs/objects/<id>.md`, one file per body.

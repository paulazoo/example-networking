# Docs
- For running closed-loop experiments on a Bruker 2p. Raw frames stream off the microscope and are processed in real-time to affect experiment runs.

- Three main machines, not including additional custom MCUs as needed:

| Program | Runs on | Role |
|---|---|---|
| `main_twophoton.cpp` | Windows PC running Bruker PrairieView software | Gets raw frames from PrairieView and sends each frame to server as binary |
| `main_fortyninety.cpp` | x86 (AVX2) Linux | Boost.Beast WS server. Processes frames, runs trial state machine, broadcasts events, logs stuff |
| `main_rpibeta.cpp` | RPi | Solenoid controls and audio tones |

- `TrialEngine` is an asynchronous state machine serialized on a Boost.Asio strand, so needs no locks. Waits until every required client has connected then broadcast `a` to start after which:

| Phase | Default duration | Broadcast | What happens |
|---|---|---|---|
| p0: inter-trial interval | 10–30 s, random | `l` | |
| p1: stimulus | 0.75 s | `m`, `q` | `q` triggers optogenetic stimulation |
| p2: response window | 2 s | `n` | For each ROI, reaching feedback level 4 earns one reward unit and level 9 earns another (0–4 total) |
| p3: outcome | 3 s | `o`, `rN` | The Pi delivers N water pulses. `r0` plays white noise instead. |
| end of trial | | `p` | |

- Every broadcast event and every sensor reading (`f`–`i` messages) is logged with ms timestamps to binary files under `../data/stimtrain_data/`. A background consumer thread batches disk I/O. Each processed frame is saved as `images/image_N.bin` like so: int32 height, int32 width, and then the uint16 pixels.

- `config.json` holds configuration settings. Defaults are in `src/core/config.h`.

# Reqs
- C++17 and Boost (Asio, Beast). [nlohmann/json](https://github.com/nlohmann/json) and [stb_image_write](https://github.com/nothings/stb) are included in `src/`.
- **fortyninety:** FFTW3, OpenMP, and a AVX2 CPU
- **twophoton:** Windows and Bruker PrairieView
- **rpibeta:** pigpio and `aplay`. Remember to stop the pigpio daemon first (`sudo killall pigpiod`)

# Repo structure
```
src/
├── main_fortyninety.cpp   # server entry point
├── main_twophoton.cpp     # microscope client entry point
├── main_rpibeta.cpp       # rpi client entry point
├── core/                  # config loading, machine specific session classes
├── trial_engine/          # trial state stuff
├── image_processing/      # image processing stuff
├── networking/            # WS server and clients, TCP clients, data buffers
├── prairieview/           # PrairieView client
├── timing/                # ms epoch timing
├── value_saving/          # binary value saving
└── image_saving/          # binary image saving
tones/                     # tones played by rpi
```
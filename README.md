# NeuroArm

## Helpful Docs
- [Cortex API Documentation](https://emotiv.gitbook.io/cortex-api)
- [Insight 2 User Manual](https://emotiv.gitbook.io/insight-2-user-manual/)

## External Sources used
- [cortex API example](https://github.com/Emotiv/cortex-example)

## Configuring the project
If you have not created a virtual environment for this project, do so by typing the following commands to the terminal (here using shell as an example):

`python -m venv .venv`

If the virtual environment is set, setup the environment by

`source .venv/bin/activate` on POSIX/POSIX-like systems

`source .venv/Scripts/activate` on Windows

Install all packages with

`pip install -r requirements.txt`

Setup Client ID and Client Secrets with

`python setup_env.py`

## Running the project

Run `pythom -m neuroarm -h` to see a usage guide.

The only required argument is `--serial-port`. This specifies the serial port the Arduino board is connected to.
For Windows this should look something like `COM5`. Find the correct port via Arduino IDE.

`--headset-id` may also need to be specified if more than one device is configured in Emotiv Launcher.
The headset ID should look something like `INSIGHT2-AAAA0000`.

## Installing the project

For more convenient execution, one may want to install this project locally.

Run `pip install -e .` to install the project as a script.

After this, running `neuroarm` also executes the project.

## Testing

For example, to see details for the mental command logging test script, run
`python -m tests.com_logging -h`

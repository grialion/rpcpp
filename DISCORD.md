# Setting up Discord libs and headers
You need Discord's Game SDK to compile and run RPC++.
## Automated way
There's a script called `setup.sh`, which will download an unzip Discord Game SDK properly. The script need unzip to be installed!
## Manual way
Alternatively you can do it yourself if you don't want to use the script or it does not work.
### Steps
1. Download [Discord Game SDK](https://dl-game-sdk.discordapp.net/2.5.6/discord_game_sdk.zip)
1. Extract the downloaded zip file
1. Copy the files from the extracted `cpp` folder to the project's `discord` folder
2. Copy the files from the extracted `lib/x86_64` folder to the project's lib folder

---
Since I don't know what license the SDK has, I did not include it in this repository.
#!/bin/bash

if ! command -v unzip &> /dev/null
then
    echo "unzip is required to unzip the downloaded file"
    exit
fi

rm -rf tmp
rm -rf lib
rm -rf src/discord
mkdir tmp
mkdir lib
mkdir -p src/discord
cd tmp
wget "https://dl-game-sdk.discordapp.net/3.2.1/discord_game_sdk.zip"
unzip discord*.zip
cp lib/x86_64/* ../lib/
cp cpp/* ../src/discord/
cd ../src/discord/

# for some stupid reason you can't compile discord unless the std:: integer types are removed lol
sed s/std::int/int/g -i *.*
sed s/std::uint/uint/g -i *.*

echo "Successfully set up Discord Game SDK"
#!/bin/bash
read -p "Enter file name: " filename
read -p "Enter seconds to record: " time
read -p "Upload to cloud? (Y/N): " -n 1 -r confirm
echo 
./sample256 -t ${time} -f ${filename}.wav -m ${filename}.mp3 -p > ${filename}.txt

if [[ ${confirm} =~ ^[Yy]$ ]]
then
	./upload_cloud 913130267241-nj0ir2m77a72pivo5krdaq1pivcobnqt.apps.googleusercontent.com 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 ${filename}.mp3 
fi

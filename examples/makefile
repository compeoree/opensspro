#  Copyright (c) 2016 Louis McCarthy
#  All rights reserved.
#
#  Licensed under MIT License, see LICENSE for full license text
#
#  This is a basic make file thats purporse is to show build dependencies and not makefile fu


CC=g++
CFLAGS=-c -Wall -D VERBOSE
OUTPUT_FOLDER=build

help:
	@echo "Compile the examples..."
	@echo ""
	@echo "      all         -  Compile all examples"
	@echo "      clean       -  Remove all build files"
	@echo ""
	@echo "      opensspro   -  Build the main library"
	@echo "      printStatus -  Connect to camera and print status bytes"
	@echo "      capture     -  Expose the CCD for 120 seconds"
	@echo "      cancel      -  Cancel the capture"
	@echo "      parser      -  Parse the raw image file and output useful statistics"
	@echo ""


# Make everything
all: printStatus capture cancel parser

	
# Create the build folder so we keep the repo clean
setup:
	mkdir -p $(OUTPUT_FOLDER)


# Build the library
# Requires the build folder, so use 'setup' as a dependency
opensspro: setup
	$(CC) $(CFLAGS) ../src/opensspro.cpp -lusb-1.0 -o $(OUTPUT_FOLDER)/opensspro.o	


# Prints the camera status packet
# Requires the build folder and the opensspro library, so set them as dependencies
printStatus: setup opensspro
	$(CC) $(CFLAGS) status_test.cpp -lusb-1.0 -o $(OUTPUT_FOLDER)/status_test.o
	$(CC) $(OUTPUT_FOLDER)/status_test.o $(OUTPUT_FOLDER)/opensspro.o -lusb-1.0 -o $(OUTPUT_FOLDER)/printStatus


capture: setup opensspro
	$(CC) $(CFLAGS) capture_test.cpp -lusb-1.0 -o $(OUTPUT_FOLDER)/capture_test.o
	$(CC) $(OUTPUT_FOLDER)/capture_test.o $(OUTPUT_FOLDER)/opensspro.o -lusb-1.0 -o $(OUTPUT_FOLDER)/capture

cancel: setup opensspro
	$(CC) $(CFLAGS) cancel_test.cpp -lusb-1.0 -o $(OUTPUT_FOLDER)/cancel_test.o
	$(CC) $(OUTPUT_FOLDER)/cancel_test.o $(OUTPUT_FOLDER)/opensspro.o -lusb-1.0 -o $(OUTPUT_FOLDER)/cancel

parser: setup
	$(CC) $(CFLAGS) parseRawImage.cpp -lusb-1.0 -o $(OUTPUT_FOLDER)/parseRawImage.o
	$(CC) $(OUTPUT_FOLDER)/parseRawImage.o -lusb-1.0 -o $(OUTPUT_FOLDER)/parser


# Undo undo undo
clean:
	rm -rf $(OUTPUT_FOLDER)

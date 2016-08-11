/*
 * Copyright 2016 Hannes Ellinger
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <getopt.h>
#include "ubertooth.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>

#define DEPTH 175
#define FONT_SIZE 12

// The window we'll be rendering to
SDL_Window* gWindow = NULL;
// The window renderer
SDL_Renderer* gRenderer = NULL;

int w, h;
int low_dbm = -120;
int high_dbm = 0;
int low_freq = 2400;
int high_freq = 2500;

int8_t rssi_values[DEPTH][600];
int rssi_index = 0;

int window_init()
{
	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}

	//Initialize SDL_TTF
	TTF_Init();

	//Create window
	gWindow = SDL_CreateWindow( "Ubertooth spectrum analyzer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_RESIZABLE );
	if( gWindow == NULL )
	{
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}

	//Create renderer for window
	gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
	if( gRenderer == NULL )
	{
		printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
		return -1;
	}

	//Initialize renderer color
	SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor( gRenderer, 0, 0, 0, 255 );
	SDL_RenderClear(gRenderer);

	return 0;
}

void window_close()
{
	//Destroy window
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
	gRenderer = NULL;
	gWindow = NULL;

	TTF_Quit();

	//Quit SDL subsystems
	SDL_Quit();
}

int dbm_to_y(int dbm)
{
	return (high_dbm - dbm) * h / (high_dbm - low_dbm);
}

int mhz_to_x(int freq)
{
	return (freq - low_freq) * w / (high_freq - low_freq);
}

void drawText(const char* text, int x, int y)
{
	// Text
	TTF_Font* Sans = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", FONT_SIZE);
	SDL_Color White = {127, 127, 127, 255};
	SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, text, White);
	SDL_Texture* Message = SDL_CreateTextureFromSurface(gRenderer, surfaceMessage);

	SDL_Rect Message_rect; //create a rect
	Message_rect.x = x;  //controls the rect's x coordinate
	Message_rect.y = y; // controls the rect's y coordinte

	SDL_QueryTexture(Message, NULL, NULL, &Message_rect.w, &Message_rect.h);

	//Mind you that (0,0) is on the top left of the window/screen, think a rect as the text's box, that way it would be very simple to understance

	//Now since it's a texture, you have to put RenderCopy in your game loop area, the area where the whole code executes

	SDL_RenderCopy(gRenderer, Message, NULL, &Message_rect); //you put the renderer's name first, the Message, the crop size(you can ignore this if you don't want to dabble with cropping), and the rect which is the size and coordinate of your texture

	SDL_FreeSurface(surfaceMessage);
	TTF_CloseFont(Sans);
}

void drawReticle()
{
	static SDL_Rect area;
	static SDL_Texture* reticle;

	SDL_GetWindowSize(gWindow, &w, &h);

	if (area.w != w || area.h != h)
	{
		area.x=0;
		area.y=0;
		area.w=w;
		area.h=h;

		SDL_DestroyTexture(reticle);
		reticle = SDL_CreateTexture( gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h );
		SDL_SetRenderTarget( gRenderer, reticle );
		SDL_SetTextureBlendMode(reticle, SDL_BLENDMODE_BLEND);

		//Clear screen
		SDL_SetRenderDrawColor( gRenderer, 0, 0, 0, 255 );
		SDL_RenderFillRect(gRenderer, &area);

		for (int i=low_dbm; i<=high_dbm; i+=20)
		{
			SDL_SetRenderDrawColor( gRenderer, 0, 0, 127, 255 );
			SDL_RenderDrawLine( gRenderer, 0, dbm_to_y(i), w, dbm_to_y(i));
			char text[8];
			sprintf(text, "%d dBm", i);
			drawText(text, 0, dbm_to_y(i)-FONT_SIZE-1);
		}
		for (int i=low_freq; i<=high_freq; i+=10)
		{
			SDL_SetRenderDrawColor( gRenderer, 0, 0, 127, 255 );
			SDL_RenderDrawLine( gRenderer, mhz_to_x(i), 0, mhz_to_x(i), h);
			char text[9];
			sprintf(text, "%d MHz", i);
			drawText(text, mhz_to_x(i), 0);
		}

		SDL_SetRenderTarget(gRenderer, NULL );
	}

	SDL_RenderCopy(gRenderer, reticle, NULL, &area);
	SDL_SetTextureAlphaMod(reticle, 10);
}

void drawLine()
{
	int8_t* max = (int8_t*)malloc(high_freq-low_freq+1 * sizeof(int8_t));
	memset(max, -128, high_freq-low_freq+1);

	for (int i=0; i<high_freq-low_freq+1; i++) {
		for (int j=0; j<DEPTH; j++) {
			if (max[i] < rssi_values[j][i])
				max[i] = rssi_values[j][i];
		}
	}

	for (int i=0; i<high_freq-low_freq; i++)
	{
		SDL_SetRenderDrawColor( gRenderer, 255, 255, 255, 255 );
		SDL_RenderDrawLine( gRenderer, mhz_to_x(i+low_freq), dbm_to_y(rssi_values[rssi_index][i]), mhz_to_x(i+low_freq+1), dbm_to_y(rssi_values[rssi_index][i+1]));
		SDL_SetRenderDrawColor( gRenderer, 0, 255, 0, 255 );
		SDL_RenderDrawLine( gRenderer, mhz_to_x(i+low_freq), dbm_to_y(max[i]), mhz_to_x(i+low_freq+1), dbm_to_y(max[i+1]));
	}
}

void cb_specan_gui(ubertooth_t* ut __attribute__((unused)), void* args __attribute__((unused)))
{
	usb_pkt_rx rx = fifo_pop(ut->fifo);

	/* process each received block */
	for (int i = 0; i < DMA_SIZE-2; i += 3)
	{
		uint16_t frequency = (rx.data[i] << 8) | rx.data[i + 1];
		int8_t rssi = (int8_t)rx.data[i + 2];

		rssi_values[rssi_index][frequency-low_freq] = rssi - 54;

		// end of a frame
		if (frequency == high_freq)
		{
			drawReticle();

			drawLine();

			//Update screen
			SDL_RenderPresent( gRenderer );

			rssi_index = (rssi_index+1) % DEPTH;
		}
	}
}

static void usage(FILE *file)
{
	fprintf(file, "ubertooth-specan - output a continuous stream of signal strengths\n");
	fprintf(file, "Usage:\n");
	fprintf(file, "\t-h this help\n");
	fprintf(file, "\t-l lower frequency (default 2400)\n");
	fprintf(file, "\t-u upper frequency (default 2500)\n");
	fprintf(file, "\t-U<0-7> set ubertooth device to use\n");
}

static void handle_events(ubertooth_t* ut)
{
	// SDL event handler
	SDL_Event e;

	// Handle events on queue
	while( SDL_PollEvent( &e ) != 0 )
	{
		// User requests quit
		if( e.type == SDL_QUIT )
		{
			ut->stop_ubertooth = 1;
		}

		// Window resized
		if( e.type == SDL_WINDOWEVENT_SIZE_CHANGED || e.type == SDL_WINDOWEVENT_RESIZED )
		{
			SDL_SetRenderDrawColor( gRenderer, 0, 0, 0, 255 );
			SDL_RenderClear(gRenderer);
		}
	}
}

int main(int argc, char *argv[])
{
	int opt, r = 0;
	char ubertooth_device = -1;

	ubertooth_t* ut = NULL;

	memset(rssi_values, -128, sizeof(rssi_values));

	while ((opt=getopt(argc,argv,"h:l::u::U:")) != EOF) {
		switch(opt) {
		case 'l':
			if (optarg)
				low_freq= atoi(optarg);
			else
				printf("lower: %d\n", low_freq);
			break;
		case 'u':
			if (optarg)
				high_freq= atoi(optarg);
			else
				printf("upper: %d\n", high_freq);
			break;
		case 'U':
			ubertooth_device = atoi(optarg);
			break;
		case 'h':
			usage(stdout);
			return 0;
		default:
			usage(stderr);
			return 1;
		}
	}

	ut = ubertooth_start(ubertooth_device);

	if (ut == NULL) {
		usage(stderr);
		return 1;
	}

	r = ubertooth_check_api(ut);
	if (r < 0)
		return 1;

	/* Clean up on exit. */
	register_cleanup_handler(ut, 0);

	//Start up SDL and create window
	r = window_init();
	if (r < 0)
		return r;

	// init USB transfer
	r = ubertooth_bulk_init(ut);
	if (r < 0)
		return r;

	r = ubertooth_bulk_thread_start();
	if (r < 0)
		return r;

	// tell ubertooth to start specan and send packets
	r = cmd_specan(ut->devh, low_freq, high_freq);
	if (r < 0)
		return r;

	// receive and process each packet
	while(!ut->stop_ubertooth) {
		ubertooth_bulk_receive(ut, cb_specan_gui, NULL);
		handle_events(ut);
	}

	window_close();

	ubertooth_bulk_thread_stop();

	ubertooth_stop(ut);
	fprintf(stderr, "Ubertooth stopped\n");

	return 0;
}

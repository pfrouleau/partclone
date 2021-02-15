/**
 * progress.c - part of Partclone project
 *
 * Copyright (c) 2007~ Thomas Tsai <thomas at nchc org tw>
 *
 * progress bar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "config.h"
#include "progress.h"
#include "gettext.h"
#define _(STRING) gettext(STRING)
#include "partclone.h"

#ifdef HAVE_LIBNCURSESW
#include <ncurses.h>
extern WINDOW *p_win;
extern WINDOW *bar_win;
extern WINDOW *tbar_win;
int color_support = 1;
int BUFSIZE = 50;
#endif

int PUI;
unsigned long RES=0;

/// initial progress bar
extern void progress_init(struct progress_bar *prog, unsigned long long start, unsigned long long stop, unsigned long long total, int flag, int size)
{
    memset(prog, 0, sizeof(progress_bar));

    prog->start = start;
    prog->stop = stop;
    prog->total = total;

    prog->initial_time = time(0);
    prog->resolution_time = time(0);
    prog->interval_time = 1;
    if (RES){
        prog->interval_time = RES;
    }
    prog->block_size = size;
    prog->block_count = stop - start; // can give zero, ex: swap
    prog->rate = 0.0;
    prog->pui = PUI;
    prog->flag = flag;
}

/// open progress interface
extern int open_pui(int pui, unsigned long res){
    int tui = 0;
    if (pui == NCURSES){
        tui = open_ncurses();
        if (tui == 0){
            close_ncurses();
	    PUI = TEXT;
	    pui = TEXT;
        }
    } else if (pui == DIALOG){
        tui = 1;
    }
    PUI = pui;
    RES = res;
    return tui;
}

/// close progress interface
extern void close_pui(int pui){
    if (pui == NCURSES){
        close_ncurses();
    }
}

extern void update_pui(struct progress_bar *prog, unsigned long long copied, unsigned long long current, int done){

    if (done != 1) {
	if ((difftime(time(0), prog->resolution_time) < prog->interval_time) && copied != 0)
	    return;

    }
    if (prog->pui == NCURSES)
        Ncurses_progress_update(prog, copied, current, done);
    else if (prog->pui == TEXT)
        progress_update(prog, copied, current, done);
}

static void format_duration(char *buffer, int buf_size, time_t duration)
{
    struct tm *tm;

    if (duration > 86400) {
	int flown = ((int)duration / 3600);

	if (flown > 999) {
	    flown = 999;
	}

	snprintf(buffer, buf_size, " > %3i hrs ", flown);
    } else {
	tm = gmtime(&duration);
	strftime(buffer, buf_size, "%H:%M:%S", tm);
    }
}

static void format_elapsed(prog_stat_t *prog_stat, time_t duration)
{
    format_duration(prog_stat->Eformated, sizeof(prog_stat->Eformated), duration);
}

static void format_remaining(prog_stat_t *prog_stat, time_t duration, int done)
{
    if (done) {
	prog_stat->percent = 100;

	duration = (time_t)0;
	format_duration(prog_stat->Rformated, sizeof(prog_stat->Rformated), duration);

	return;
    }

    format_duration(prog_stat->Rformated, sizeof(prog_stat->Rformated), duration);
}

static void calculate_speed(struct progress_bar *prog, unsigned long long copied, unsigned long long current, int done, prog_stat_t *prog_stat){
    uint64_t speedps = 1;
    uint64_t speed = 1;
    double dspeed = 1.0;
    float percent = 1.0;
    time_t remained;
    time_t elapsed;
    uint64_t gbyte=1000000000.0;
    uint64_t mbyte=1000000;
    uint64_t kbyte=1000;

    if (prog->block_count == 0) {
	percent = 99.99;
    } else {
	percent = 100.0f * copied / (float)(prog->block_count);
	if (percent <= 0)
	    percent = 1;
	else if (percent >= 100)
	    percent = 99.99;
    }

    prog_stat->percent = percent;

    elapsed = (time(0) - prog->initial_time);
    if (elapsed <= 0)
	elapsed = 1;

    speedps = prog->block_size * copied / elapsed;
    speed = speedps * 60.0;

    if (speed >= gbyte){
	dspeed = (double)speed / (double)gbyte;
	strncpy(prog_stat->speed_unit, "GB", 3);
    }else if (speed >= mbyte){
	dspeed = (double)speed / (double)mbyte;
	strncpy(prog_stat->speed_unit, "MB", 3);
    }else if (speed >= kbyte){
	dspeed = (double)speed / (double)kbyte;
	strncpy(prog_stat->speed_unit, "KB", 3);
    }else{
	dspeed = speed;
	strncpy(prog_stat->speed_unit, "byte", 5);
    }

    prog_stat->speed = dspeed;

    prog_stat->total_percent = 100.0f * current / (float)(prog->total);

    remained = (time_t)((elapsed/percent*100) - elapsed);
    format_remaining(prog_stat, remained, done);
    format_elapsed(prog_stat, elapsed);
}

/// update information at progress bar
extern void progress_update(struct progress_bar *prog, unsigned long long copied, unsigned long long current, int done)
{
    char clear_buf = ' ';
    prog_stat_t prog_stat;

    memset(&prog_stat, 0, sizeof(prog_stat_t));
    calculate_speed(prog, copied, current, done, &prog_stat);

    if (done != 1){
	prog->resolution_time = time(0);
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fprintf(stderr, _("\r%80c\rElapsed: %s, Remaining: %s, Completed: %6.2f%%"), clear_buf, prog_stat.Eformated, prog_stat.Rformated, prog_stat.percent);

	if((prog->flag == IO) || (prog->flag == NO_BLOCK_DETAIL))
	    fprintf(stderr, _(", %6.2f%s/min,"), prog_stat.speed, prog_stat.speed_unit);
	if(prog->flag == IO)
	    fprintf(stderr, "\n\r%80c\rcurrent block: %10Lu, total block: %10Lu, Complete: %6.2f%%%s\r", clear_buf, current, prog->total, prog_stat.total_percent, "\x1b[A");
    } else {
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fprintf(stderr, _("\r%80c\rElapsed: %s, Remaining: %s, Completed: 100.00%%"), clear_buf, prog_stat.Eformated, prog_stat.Rformated);
	if((prog->flag == IO) || (prog->flag == NO_BLOCK_DETAIL))
	    fprintf(stderr, _(", Rate: %6.2f%s/min,"), prog_stat.speed, prog_stat.speed_unit);
	if(prog->flag == IO)
	    fprintf(stderr, "\n\r%80c\rcurrent block: %10Lu, total block: %10Lu, Complete: 100.00%%\r", clear_buf, current, prog->total);

        fprintf(stderr, _("\nTotal Time: %s, "), prog_stat.Eformated);
	if(prog->flag == IO)
	    fprintf(stderr, _("Ave. Rate: %6.1f%s/min, "), prog_stat.speed, prog_stat.speed_unit);
        fprintf(stderr, _("%s"), "100.00% completed!\n");
    }
}

/// update information at ncurses mode
extern void Ncurses_progress_update(struct progress_bar *prog, unsigned long long copied, unsigned long long current, int done)
{
#ifdef HAVE_LIBNCURSESW

    char *block = "                                                                    ";
    int x = 0;
    char blockbuf[BUFSIZE];
    prog_stat_t prog_stat;

    memset(&prog_stat, 0, sizeof(prog_stat_t));
    calculate_speed(prog, copied, current, done, &prog_stat);

    /// set bar color
    init_pair(4, COLOR_RED, COLOR_RED);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);
    init_pair(6, COLOR_WHITE, COLOR_RED);
    werase(p_win);
    werase(bar_win);

    if (done != 1){
	prog->resolution_time = time(0);

        mvwprintw(p_win, 0, 0, _("Elapsed: %s Remaining: %s ") , prog_stat.Eformated, prog_stat.Rformated);
	if((prog->flag == IO) || (prog->flag == NO_BLOCK_DETAIL))
	    mvwprintw(p_win, 0, 40, _("Rate: %6.2f%s/min"), prog_stat.speed, prog_stat.speed_unit);
	if (prog->flag == IO)
	    mvwprintw(p_win, 1, 0, _("Current Block: %llu  Total Block: %llu ") , current, prog->total);

	if (prog->flag == IO)
	    mvwprintw(p_win, 3, 0, "Data Block Process:");
	else if (prog->flag == BITMAP)
	    mvwprintw(p_win, 3, 0, "Calculating Bitmap Process:");
	wattrset(bar_win, COLOR_PAIR(4));
	x = snprintf(blockbuf, BUFSIZE, "%.*s",  (unsigned int)(prog_stat.percent*0.5), block);
	if ( x < 0 )
	    fprintf(stderr, "ncurses update error\n");
        mvwprintw(bar_win, 0, 0, "%s", blockbuf);
        wattroff(bar_win, COLOR_PAIR(4));
        mvwprintw(p_win, 4, 52, "%6.2f%%", prog_stat.percent);

	if (prog->flag == IO) {
	    werase(tbar_win);
	    mvwprintw(p_win, 6, 0, "Total Block Process:");
	    wattrset(tbar_win, COLOR_PAIR(4));
	    x = snprintf(blockbuf, BUFSIZE, "%.*s",  (unsigned int)(prog_stat.total_percent*0.5), block);
	    if ( x < 0 )
		fprintf(stderr, "ncurses update error\n");
	    mvwprintw(tbar_win, 0, 0, "%s", blockbuf);
	    wattroff(tbar_win, COLOR_PAIR(4));
	    mvwprintw(p_win, 7, 52, "%6.2f%%", prog_stat.total_percent);
	}


	wrefresh(p_win);
        wrefresh(bar_win);
        wrefresh(tbar_win);
    } else {
        mvwprintw(p_win, 0, 0, _("Total Time: %s Remaining: %s "), prog_stat.Eformated, prog_stat.Rformated);
	if((prog->flag == IO) || (prog->flag == NO_BLOCK_DETAIL))
	    mvwprintw(p_win, 1, 0, _("Ave. Rate: %6.2f%s/min"), prog_stat.speed, prog_stat.speed_unit);

	if (prog->flag == IO)
	    mvwprintw(p_win, 3, 0, "Data Block Process:");
	else if (prog->flag == BITMAP)
	    mvwprintw(p_win, 3, 0, "Calculating Bitmap Process:");
        wattrset(bar_win, COLOR_PAIR(4));
	mvwprintw(bar_win, 0, 0, "%50s", " ");
        wattroff(bar_win, COLOR_PAIR(4));
        mvwprintw(p_win, 4, 52, "100.00%%");

	if (prog->flag == IO) {
	    werase(tbar_win);
	    mvwprintw(p_win, 6, 0, "Total Block Process:");
	    wattrset(tbar_win, COLOR_PAIR(4));
	    mvwprintw(tbar_win, 0, 0, "%50s", " ");
	    wattroff(tbar_win, COLOR_PAIR(4));
	    mvwprintw(p_win, 7, 52, "100.00%%");
	}

        wrefresh(p_win);
        wrefresh(bar_win);
        wrefresh(tbar_win);
        refresh();
	sleep(1);
    }

#endif
}

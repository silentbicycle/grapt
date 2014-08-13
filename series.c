#include <float.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "grapt.h"

#define MIN(x, y) (x > y ? y: x)
#define MAX(x, y) (x > y ? x: y)

static void
alloc_pts(series_t *series, size_t count)
{
  if (series->pts == NULL) {
    series->pts = calloc(count, sizeof(*series->pts));
    if (series->pts == NULL) {
      perror("malloc");
      exit(1);
    }
    series->pts_used = 0;
    series->pts_alloc = count;
  }
  else {
    series->pts = realloc(series->pts, sizeof(*series->pts) * count);
    if (series->pts == NULL) {
      perror("realloc");
      exit(1);
    }
    series->pts_alloc = count;
  }
}


void
series_init(series_t *series)
{
  series->pts = NULL;
  series->pts_used = 0;
  series->pts_alloc = 0;
  series->next = NULL;
}


void
series_destroy(series_t *series)
{
  if (series == NULL) {
    return;
  }

  if (series->next != NULL) {
    series_destroy(series->next);
  }
  else {
    free(series->pts);
    free(series);
  }
}


void
series_append(series_t *series, point_t pt)
{
  if (series->pts_used < series->pts_alloc) {
    series->pts[series->pts_used++] = pt;
  }
  else {
    alloc_pts(series, series->pts_alloc == 0 ? 8: series->pts_alloc * 2);
    series->pts[series->pts_used++] = pt;
  }
}


int
series_copy(series_t *series, series_t *to)
{
  to->pts_alloc = series->pts_alloc;
  to->pts_used = series->pts_used;
  to->next = NULL;
  
  to->pts = malloc(sizeof(*to->pts) * to->pts_alloc);
  if (to->pts == NULL) {
    return -1;
  }
  
  bcopy(series->pts, to->pts, sizeof(*to->pts) * to->pts_alloc);
  return 0;
}


void
series_transform(series_t *series, point_t (*funcp)(point_t x))
{
  int i;
  for (i = 0; i < series->pts_used; i++) {
    series->pts[i] = (*funcp)(series->pts[i]);
  }
}

void
series_window(series_t *series, window_t *window)
{
  double val;
  int i = 0;

  if (series->pts_used > 0) {
    window->min_x = series->pts[0].x;
    window->max_x = series->pts[0].x;
    window->min_y = series->pts[0].y;
    window->max_y = series->pts[0].y;
  }

  for (i = 0; i < series->pts_used; i++) {
    if (series->pts[i].x < window->min_x) {
      window->min_x = series->pts[i].x;
    }
    if (series->pts[i].y < window->min_y) {
      window->min_y = series->pts[i].y;
    }
    if (series->pts[i].x > window->max_x) {
      window->max_x = series->pts[i].x;
    }
    if (series->pts[i].y > window->max_y) {
      window->max_y = series->pts[i].y;
    }
  }
}


void
series_read(series_t *series, FILE *in)
{
  char buf[1024];
  point_t pt;
  int x = 0;
  int count, required = -1;
  double left, right;
  

  fgets(buf, 1024, in);
  while (!feof(in)) {
    count = sscanf(buf, "%lf %lf\n", &left, &right);
    if (required == -1) {
      required = count;
    }

    if (required != count) {
      fprintf(stderr, "Required %d values. Got %d\n", required, count);
      exit(1);
    }
    else if (count == 2) {
      pt.x = left;
      pt.y = right;
    }
    else if (count == 1) {
      pt.x = x++;
      pt.y = left;
    }
    else {
      fprintf(stderr, "No values found! Aborting\n");
      exit(1);
    }

    series_append(series, pt);
    fgets(buf, 1024, in);
  }
}


void
data_window(series_t *series, window_t *window)
{
  window_t current;
  while (series != NULL) {
    series_window(series, &current);
    if (current.max_x > window->max_x) {
      window->max_x = current.max_x;
    }
    if (current.max_y > window->max_y) {
      window->max_y = current.max_y;
    }
    if (current.min_x < window->min_x) {
      window->min_x = current.min_x;
    }
    if (current.min_y < window->min_y) {
      window->min_y = current.min_y;
    }
    
    series = series->next;
  }
}
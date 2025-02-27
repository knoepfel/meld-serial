read_times <- function(fname) {
  read_tsv(fname, col_types = "ciccii") |>
    mutate(time = as.numeric(lubridate::hms(time)),
           thread = as.factor(thread),
           event = as.factor(event),
           node = as.factor(node)) |>
    mutate(time = time-min(time)) |>
    mutate(time = 1000 * time)
}

scale_x_log10_nice <- function(...)
{
  scale_x_log10(breaks=scales::trans_breaks("log10", function(x) 10^x),
                labels=scales::trans_format("log10", scales::math_format(10^.x)))
}

scale_y_log10_nice <- function(...)
{
  scale_y_log10(breaks=scales::trans_breaks("log10", function(x) 10^x),
                labels=scales::trans_format("log10", scales::math_format(10^.x)))
}

# Produce a vector of "black" or "white" for each given color, so that they
# can be used as labels that contrast with the given color.
get_contrast_color <- function(color) {
  rgb <- col2rgb(color) / 255
  luminance <- 0.2126 * rgb[1] + 0.7152 * rgb[2] + 0.0722 * rgb[3]
  if (luminance > 0.5) "black" else "white"
}

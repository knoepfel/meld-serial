# Analysis of Serial Node Timing


- [<span class="toc-section-number">1</span>
  Introduction](#introduction)
- [<span class="toc-section-number">2</span> Analysis of thread
  occupancy](#analysis-of-thread-occupancy)
- [<span class="toc-section-number">3</span> Looking on at
  calibrations](#looking-on-at-calibrations)
- [<span class="toc-section-number">4</span> Distribution of *Source*
  task durations](#distribution-of-source-task-durations)
- [<span class="toc-section-number">5</span> Idle time between
  tasks](#idle-time-between-tasks)

## Introduction

This document provides some analysis of the timing performance of the
`serial_node` node type. It analysis a simple data flow with 7 nodes.

- One *Source* (input node) that generates a series of messages. Each
  message is represented by a single integer. As an input node, it is
  inherently serialized.
- One *Propagating* node. This node has unlimited parallelism.
- One *Histogramming* node. This node requires sole access to the *ROOT*
  token, of which there is only one.
- One *Generating* node. This node require sole access to the *GENIE*
  token, of which there is only one.
- One *Histo-Generating* node. This node requires simultaneous access to
  both the *ROOT* and *GENIE* tokens.
- Three *Calibration* nodes. These nodes are labeled *Calibration\[A\]*,
  *Calibration\[B\]*, and *Calibration\[C\]*. Each of these nodes
  requires sole access to a *DB* token. There are two *DB* tokens
  available. *DB* tokens have an associated integer ID; the values are 1
  and 13.

The *Source* node is directly connected to each of the other nodes.
There are no other connections between nodes. The tasks performed by the
system is the generation of the messages and the processing of each
message by all the other nodes in the system.

Each time a node is fired, we record two events. The *Start* event is
recorded as the first thing done within the body of the node. The node
then performs whatever work it is to do (for all but the *Source*, this
is a busy loop for some fixed time). The *Stop* event is recorded as the
last thing done within the body of the node. For each event, we record
the *thread* on which the event occurred, the *node* that was executing,
the *message* number, and the extra *data* associated with the event.
The extra data is meaningful only for the *Calibration* nodes; this data
is the *DB* token ID.

The event records are used to form the `events` data frame. Each task is
associated with both a *Start* and a *Stop* event. These times are
measured in milliseconds since the start of the first task. The `tasks`
data frame is formed by pivoting the `events` data frame to have a
single row for each task. The *duration* of each task is the difference
between the *Stop* and *Start* times, and is also recorded in
milliseconds.

The first few rows of the `tasks` data frame are shown in
<a href="#tbl-read-events" class="quarto-xref">Table 1</a> below.

<div id="tbl-read-events">

Table 1: First few rows of the `tasks` data frame.

<div class="cell-output-display">

| thread   | node             | message | data | Start |    Stop | duration |
|:---------|:-----------------|--------:|-----:|------:|--------:|---------:|
| 10505306 | Source           |       1 |    0 | 0.000 |   1.353 |    1.353 |
| 10505306 | Calibration\[C\] |       1 |    1 | 1.424 |  11.454 |   10.030 |
| 10505304 | Histogramming    |       1 |    0 | 1.442 |  11.463 |   10.021 |
| 10505305 | Propagating      |       1 |    0 | 1.480 | 151.505 |  150.025 |
| 10505307 | Calibration\[A\] |       1 |   13 | 1.497 |  11.580 |   10.083 |
| 10505296 | Source           |       2 |    0 | 1.537 |   1.557 |    0.020 |
| 10505296 | Source           |       3 |    0 | 3.117 |   3.139 |    0.022 |
| 10505301 | Generating       |       1 |    0 | 3.121 |  13.167 |   10.046 |
| 10505296 | Source           |       4 |    0 | 3.204 |   3.213 |    0.009 |
| 10505296 | Source           |       5 |    0 | 3.225 |   3.232 |    0.007 |
| 10505298 | Propagating      |       2 |    0 | 3.235 | 153.306 |  150.071 |
| 10505300 | Propagating      |       3 |    0 | 3.240 | 153.258 |  150.018 |

</div>

</div>

The graph was executed with 50 messages, on a 12-core Mac laptop.

## Analysis of thread occupancy

Our main concern is to understand what the threads are doing, and to see
if the hardware is being used efficiently. By “efficiently”, we mean
that the threads are busy doing useful work — i.e., running our tasks.
<a href="#fig-thread-busy" class="quarto-xref">Fig. 1</a> shows the
timeline of task execution for each thread.

<div id="fig-thread-busy">

![](serial_node_timing_files/figure-commonmark/fig-thread-busy-1.png)

Figure 1: Task execution timeline, showing when each thread is busy and
what it is doing. This workflow was run on a 12-core Mac laptop.

</div>

The cores are all busy until about 700 milliseconds, at which time we
start getting some idle time. This is when there are no more
*Propagating* tasks to be started; this is the only node that has
unlimited parallelism. In this view, the *Source* tasks are completed so
quickly that we cannot see them.

<a href="#fig-program-start" class="quarto-xref">Fig. 2</a> zooms in on
the first 5.0 milliseconds of the program.

<div id="fig-program-start">

![](serial_node_timing_files/figure-commonmark/fig-program-start-1.png)

Figure 2: Task execution timeline, showing the startup of the program.

</div>

We can see that the *Source* is firing many times, and with a wide
spread of durations. We also see that the *Source* is serialized, as
expected for an input node. There is some small idle time after the
first firing of the *Source*, and a considerable delay after the second
firing. After the first firing, all the activity for the *Source* moves
to a different thread. There are some idle times between source firings
that we do not understand.

In <a href="#fig-program-start-after-first"
class="quarto-xref">Figure 3</a>, we can zoom in further to see what is
happening after the first firing of the *Source*. There are delays
between the creation of a message by the *Source* and the start of the
processing of that message by one of the other nodes, and that delay is
variable.

<div id="fig-program-start-after-first">

![](serial_node_timing_files/figure-commonmark/fig-program-start-after-first-1.png)

Figure 3: Task execution timeline, showing the time after the first
source firing. The numeric label in each rectangle shows the message
number being processed by the task.

</div>

We can also zoom in on the program wind-down, as shown in
<a href="#fig-program-wind-down" class="quarto-xref">Figure 4</a>. Here
it appears we have some idle threads because there is insufficient work
to be done. We do not see any sign of unexploited parallelism.

<div id="fig-program-wind-down">

![](serial_node_timing_files/figure-commonmark/fig-program-wind-down-1.png)

Figure 4: Task execution timeline, showing the time after the first
source firing.

</div>

## Looking on at calibrations

Flowgraph seem to prefer keeping some tasks on a single thread. All of
the *Histo-generating* tasks were run on a single thread. The
calibrations are clustered onto a subset of threads, and are not
distributed evenly across those threads. This is shown in
<a href="#tbl-calibrations" class="quarto-xref">Table 2</a>.

<div id="tbl-calibrations">

Table 2: Number of calibrations of each type done by each thread.

<div class="cell-output-display">

| thread   | node             |   n |
|:---------|:-----------------|----:|
| 10505296 | Calibration\[B\] |   1 |
| 10505297 | Calibration\[A\] |  17 |
| 10505297 | Calibration\[C\] |  14 |
| 10505299 | Calibration\[A\] |  25 |
| 10505299 | Calibration\[B\] |   5 |
| 10505299 | Calibration\[C\] |  30 |
| 10505302 | Calibration\[B\] |   2 |
| 10505305 | Calibration\[C\] |   1 |
| 10505306 | Calibration\[A\] |   5 |
| 10505306 | Calibration\[B\] |  40 |
| 10505306 | Calibration\[C\] |   4 |
| 10505307 | Calibration\[A\] |   3 |
| 10505307 | Calibration\[B\] |   2 |
| 10505307 | Calibration\[C\] |   1 |

</div>

</div>

## Distribution of *Source* task durations

The body of the *Source* task consists of a single increment of an
integer. From this, one might assume that the duration of *Source* tasks
would be very similar to each other, and quite short. The data show
otherwise. Focusing on the distribution of durations for the *Source*
tasks, the variation is quite striking, as shown in
<a href="#fig-source-durations" class="quarto-xref">Figure 5</a>.

<div id="fig-source-durations">

![](serial_node_timing_files/figure-commonmark/fig-source-durations-1.png)

Figure 5: Distribution of durations for the Source tasks. Note the log
$x$ axis.

</div>

With such an extreme variation, we wonder if there may be some
interesting time structure. The *Source* generates numbers in sequence
and serially. Thus we can see time structure by plotting different
quantities as a function of either the sequential number (`message`)
(<a href="#fig-source-time-structure-by-message"
class="quarto-xref">Figure 6</a>) or as the starting time
(<a href="#fig-source-time-structure-by-start"
class="quarto-xref">Figure 7</a>).

<div id="fig-source-time-structure-by-message">

![](serial_node_timing_files/figure-commonmark/fig-source-time-structure-by-message-1.png)

Figure 6: Duration of Source tasks a function of the message number. The
color of the point indicates the thread on which the task was run.

</div>

<div id="fig-source-time-structure-by-start">

![](serial_node_timing_files/figure-commonmark/fig-source-time-structure-by-start-1.png)

Figure 7: Duration of Source tasks a function of the starting time. The
color of the point indicates the thread on which the task was run. Note
the log $x$ axis. Also note that the red point starts at time 0, but is
plotted at the edge of the graph because the log scale cannot extend to
time 0.

</div>

## Idle time between tasks

One of the concerns that some have with a system, like flowgraph, that
automates the scheduling of tasks is the amount of time that the
scheduling system takes (as opposed to the time spend doing the user’s
defined work). We can estimate this by looking at the time between the
end of one task and the start of the next task scheduled on the same
thread. We call this time the *delay* and associate it with the second
task. We also record, for each delay, whether the previous task on the
thread was run by the same node. We find this is true for the large
majority of tasks.

There are a few delays that are much longer than others, as shown in
<a href="#tbl-long-delays" class="quarto-xref">Table 3</a>.

<div id="tbl-long-delays">

Table 3: Details of the delays longer than 1 millisecond.

<div class="cell-output-display">

| thread | node | message | data | Start | Stop | duration | delay | before | same |
|:---|:---|---:|---:|---:|---:|---:|---:|:---|:---|
| 10505296 | Source | 3 | 0 | 3.117 | 3.139 | 0.022 | 1.560 | Source | TRUE |
| 10505299 | Calibration\[B\] | 49 | 13 | 629.275 | 639.281 | 10.006 | 2.468 | Calibration\[B\] | TRUE |
| 10505301 | Histo-generating | 13 | 0 | 626.827 | 636.832 | 10.005 | 1.000 | Histo-generating | TRUE |

</div>

</div>

<a href="#fig-delay-distribution" class="quarto-xref">Fig. 8</a> shows
the distribution of the lengths of the delays. They are typically tens
of microseconds. We observe no significant difference between the
distributions for delays for cases when the task in question was run by
the same node or by a different node.

<div id="fig-delay-distribution">

![](serial_node_timing_files/figure-commonmark/fig-delay-distribution-1.png)

Figure 8: Distribution of delays for all delays shorter than 1
millisecond. Note the log $x$ axis. The bottom panel shows the delays
for tasks for which the previous task on the same thread was run by the
same node. The top panel shows the delays for tasks for which the
previous task on the same thread was run by a different node.

</div>

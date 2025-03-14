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
  and 13. *Calibration\[A\]* and *Calibration\[B\]* have unlimited
  concurrency but *Calibration\[C\]* is serial. This means that, at the
  same time, we can have:
  - two *Calibration\[A\]* tasks running, or
  - two *Calibration\[B\]* tasks running, or
  - one *Calibration\[A\]* task running and one *Calibration\[B\]* task
    running, or
  - one *Calibration\[A\]* task running and one *Calibration\[C\]* task
    running, or
  - one *Calibration\[B\]* task running and one *Calibration\[C\]* task
    running.

  But we can not have two *Calibration\[C\]* tasks running at the same
  time.

The *Source* node is directly connected to each of the other nodes.
There are no other connections between nodes. The tasks performed by the
system is the generation of the messages and the processing of each
message by all the other nodes in the system.

Each time a node is fired, we record two events. A *Start* event is
recorded as the first thing done within the body of the node. The node
then performs whatever work it is to do (for all but the *Source*, this
is a busy loop for some fixed time). A *Stop* event is recorded as the
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

| thread  | node             | message | data | Start |    Stop | duration |
|:--------|:-----------------|--------:|-----:|------:|--------:|---------:|
| 7578204 | Source           |       1 |    0 | 0.000 |   1.142 |    1.142 |
| 7578204 | Calibration\[C\] |       1 |    1 | 1.209 |  11.244 |   10.035 |
| 7578208 | Source           |       2 |    0 | 1.245 |   1.386 |    0.141 |
| 7578212 | Histogramming    |       1 |    0 | 1.255 |  11.292 |   10.037 |
| 7578214 | Calibration\[B\] |       1 |   13 | 1.296 |  11.327 |   10.031 |
| 7578211 | Generating       |       1 |    0 | 1.346 |  11.764 |   10.418 |
| 7578206 | Propagating      |       1 |    0 | 1.378 | 151.434 |  150.056 |
| 7578208 | Source           |       3 |    0 | 1.420 |   1.517 |    0.097 |
| 7578213 | Propagating      |       2 |    0 | 1.438 | 151.463 |  150.025 |
| 7578208 | Source           |       4 |    0 | 1.570 |   1.601 |    0.031 |
| 7578207 | Propagating      |       3 |    0 | 1.614 | 151.638 |  150.024 |
| 7578208 | Source           |       5 |    0 | 1.625 |   1.671 |    0.046 |

</div>

</div>

The graph was executed with 50 messages, on a 12-core MacBook Pro laptop
with an Apple M2 Max chip.

## Analysis of thread occupancy

Our main concern is to understand what the threads are doing, and to see
if the hardware is being used efficiently. By “efficiently”, we mean
that the threads are busy doing useful work—i.e., running our tasks.
<a href="#fig-thread-busy" class="quarto-xref">Fig. 1</a> shows the
timeline of task execution for each thread.

<div id="fig-thread-busy">

![](serial_node_timing_files/figure-commonmark/fig-thread-busy-1.png)

Figure 1: Task execution timeline, showing when each thread is busy and
what it is doing. This workflow was run on a 12-core laptop, using 12
threads.

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
first firing of the *Source*, and a addtional delays after the next few
firings. After the first firing, all the activity for the *Source* moves
to a different thread. We do not understand the cause of the delays
between source firings, or the range of durations of the *Source* tasks.

In <a href="#fig-program-start-after-first"
class="quarto-xref">Figure 3</a>, we can zoom in further to see what is
happening after the first firing of the *Source*. There are delays
between the creation of a message by the *Source* and the start of the
processing of that message by one of the other nodes, and that delay is
variable. In this plot we see that it is message 17 that has an enormous
duration for the *Source* task: 4.889 ms.

<div id="fig-program-start-after-first">

![](serial_node_timing_files/figure-commonmark/fig-program-start-after-first-1.png)

Figure 3: Task execution timeline, showing the time after the first
source firing. The numeric label in each rectangle shows the message
number being processed by the task.

</div>

We can also zoom in on the program wind-down, as shown in
<a href="#fig-program-wind-down" class="quarto-xref">Figure 4</a>. Here
it appears we have some idle threads because there is insufficient work
to be done. We also see the effect of the serial constraint on the
*Calibration\[C\]* node. While *Calibration\[A\]* and *Calibration\[B\]*
can run simultaneously on two threads, which we see in the last part of
the plot, we see that *Calibration\[C\]* is only running on one thread
at a time. We do not see any sign of unexploited parallelism.

<div id="fig-program-wind-down">

![](serial_node_timing_files/figure-commonmark/fig-program-wind-down-1.png)

Figure 4: Task execution timeline, showing the time after the first
source firing.

</div>

## Looking on at calibrations

Flowgraph seem to prefer keeping some tasks on a single thread. All of
the *Histo-generating* tasks were run on the same thread. The same is
true for *Generating* and *Histogramming* tasks. The calibrations are
clustered onto a subset of threads, and are not distributed evenly
across those threads. This is shown in
<a href="#tbl-calibrations" class="quarto-xref">Table 2</a>.

<div id="tbl-calibrations">

Table 2: Number of calibrations of each type done by each thread.

<div class="cell-output-display">

| thread  | node             |   n |
|:--------|:-----------------|----:|
| 7578204 | Calibration\[A\] |   8 |
| 7578204 | Calibration\[B\] |  28 |
| 7578204 | Calibration\[C\] |  27 |
| 7578205 | Calibration\[A\] |   4 |
| 7578205 | Calibration\[B\] |  14 |
| 7578205 | Calibration\[C\] |  17 |
| 7578206 | Calibration\[B\] |   1 |
| 7578206 | Calibration\[C\] |   1 |
| 7578207 | Calibration\[A\] |  10 |
| 7578207 | Calibration\[B\] |   3 |
| 7578207 | Calibration\[C\] |   3 |
| 7578214 | Calibration\[A\] |   1 |
| 7578214 | Calibration\[B\] |   1 |
| 7578215 | Calibration\[A\] |  27 |
| 7578215 | Calibration\[B\] |   3 |
| 7578215 | Calibration\[C\] |   2 |

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
| 7578204 | Calibration\[C\] | 43 | 1 | 955.154 | 965.164 | 10.010 | 140.029 | Propagating | FALSE |
| 7578205 | Propagating | 29 | 0 | 453.642 | 603.658 | 150.016 | 1.544 | Propagating | TRUE |
| 7578215 | Propagating | 37 | 0 | 603.757 | 753.760 | 150.003 | 1.567 | Propagating | TRUE |

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

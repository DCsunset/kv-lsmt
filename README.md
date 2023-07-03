# KV store based on LSM Tree

This project implements a KV store based on LSM Tree.

## Design

The basic storage unit is a table (`SSTable`).
For all the tables, once a key-value pair is written,
it can't be modified anymore.
The way to modify existing keys is to overwrite previous values.
When the table's size grows to a certain threshold,
a new table is created.

For read operations, we need to go through tables from latest to oldest to search for the key.
Once it's found, we can immediately return as it's the current value.
The key doesn't exist if and only if no table contains it.

One benefit of such design is that writes can be persisted to disk by simply appending an entry in the log,
which greatly increase the efficiency of writes.
Reads that touch recently modified data will also be efficient.

Definition and implementation of the table is located in file `sstable.h` and `sshtable.cc`.
To improve the lookup process, each table is divided into index part and data part,
which are stored in different files.
The index part includes the keys and the offsets of corresponding value in the data part.
So when looking up a key, it's only necessary to read the index file until we find it.
In this way, we can preload many keys (without its data) into the memory and greatly improve the efficiency of reads.

## Crash Consistency

This data store guarantees crash consistency.
The way to implement it is that for each write, it must be persisted to disk before sending response to the API caller.
For this data store, writes can be simply appended to the index file and data file.
One detail is that length of the key or the value is written before the key or the value itself.
This makes sure that incomplete writes can be detected by checking the length and the actual size of the key or data,
which in turn guarantees crash consistency.

## Table Management

To manage a number of existing `SSTable`, a `DataStore` class is implemented.
It only stores the complete information (index and data) of current table in memory for fast read and write.
For previous tables, only indexes are kept in memory to reduce the memory usage.
When the size of current table reachs a certain threshold, it will create a new table as the current table,
and following write operations will be stored in the new table.

## Future Work

Keeping adding new tables is not a good idea when old keys are modified frequenetly.
This results in a lot of old entries that will not be read anymore.
To solve this issue, periodic consolidation of the old tables is necessary for garbage collection.

## Performance

In the experiment,
the keys each operation accesses follows Zipf distribution.
When `isSkewed` is 0, the $\theta$ parameter in Zipf is 0.
Otherwise, $\theta = 0$.

The performance is shown in the following table.
(when isSkewed is 1, it uses Zipf distribution:

| # of Threads | Read Ratio (%) | isSkewed | Throughput (op/s) |
|--------------|----------------|----------|-------------------|
| 1            | 0              | 0        | 102k              |
| 1            | 50             | 0        | 201k              |
| 1            | 100            | 0        | 2369k             |
| 1            | 0              | 1        | 136k              |
| 1            | 50             | 1        | 241k              |
| 1            | 100            | 1        | 1429k             |
| 4            | 0              | 0        | 25k               |
| 4            | 50             | 0        | 79k               |
| 4            | 100            | 0        | 5197k             |
| 4            | 0              | 1        | 29k               |
| 4            | 50             | 1        | 94k               |
| 4            | 100            | 1        | 3410k             |

As we can conclude from the table:

- When read ratio is low, using single thread outperforms multiple threads.
  Otherwise, using multiple threads has higher throughput.
- When read ration is low, skewed distribution has better performance.
  Otherwise, non-skewed distribution performs better.

For the first observation, the reason is that using multiple threads will need to acquire the write lock frequenetly.
The exclusive lock prevents concurrent writes and introduces unnecessary overhead.

For the second observation, the reason is under skewed distribution
operations tend to access the same set of keys frequenetly,
If there are more writes, then reads are very likely to access the data in the current table,
without fetching data from disk, thus outperforms the non-skewed workload.
However, if there are more reads, it's more likely that more reads will access the key in previous tables,
so the performance is worse compared to a non-skewed workload.



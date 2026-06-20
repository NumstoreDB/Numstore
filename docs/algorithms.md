An Introduction to Rope+Trees
=============================

A Rope+Tree is a tree data structure optimized for array data. More generally, a Rope+Tree is a B+Tree (REF) but instead of storing inner node values pointing to index locations on the lowest layer, Rope+Tree's store the count of elements of their child nodes. 

Basic Operations
----------------

There are four first class operations on a rope+tree:

  1. Insert(byte offset, data)
  2. Write(byte offset, stride, data) 
  3. Read(byte offset, stride, number of elements, destination buffer)
  4. Remove(byte offset, stride, number of elements, destination buffer)

Insert(byte offset, data)
-------------------------
The insert operation takes an input array of bytes of length n, and "pushes" it into the index "byte offset" of an array of length m. 

Write(byte offset, stride, data) 
--------------------------------
The write operation takes an input array of bytes of length n, and "overwrites" data in an array of length m. 

 Write Overflow
  It's up to the developer how they want to implement overflow. 
  Smart Files implement overflow using the following algorithm:

     If offset + n > m then:
       If stride == 1 then:
         write(offset, data[0:(offset + n) - m]) // Write as much data as you can 
         insert(-1, data[(offset + n) - m: n])   // Insert the remaining data
       else: raise error("Index out of bounds")

  However, from this point on, the "write" algorithm will strictly refer to in bounds 
  writes. That is, offset + n will always be < m from here on out

Read(byte offset, stride, number of elements, destination buffer)
-----------------------------------------------------------------

The read operation reads data from the source buffer into the destination buffer

Remove(byte offset, stride, number of elements, destination buffer)
-------------------------------------------------------------------

The remove operation removes data from the source buffer into the destination buffer. 
You may also optionally pass a destination buffer that keeps track of the data that was removed 

Algorithms For Basic Operations 
===============================

We begin with the read and write algorithms because they do not modify the internals 
of the rope+tree. That's because neither read nor write change the existing structure of 
the underlying binary tree.

Seek
----
The first step of every algorithm is to seek to the desired byte offset.

Seek takes a byte offset and walks the tree from the root down to the leaf level. 
At each inner node, it computes prefix sums of the child byte-counts and finds 
the smallest child index whose cumulative sum exceeds the offset. It subtracts 
the preceding sum from the offset and descends into that child, optionally saving 
the current page onto a stack for later rebalancing. This repeats until a leaf 
page is reached, at which point the remaining offset is clamped to the number 
of bytes used on that page, giving the local index within the leaf. The result 
is a page handle and a local index representing the exact position in the data 
corresponding to the original byte offset.

```
FUNCTION Seek(byteOffset, saveStack):
    page = fetchRoot()

    LOOP forever:

        SWITCH page.type:

            CASE "Inner Node":
                n = page.childCount

                // S[0] = 0, S[i] = keys[0] + keys[1] + ... + keys[i-1]
                S = iterativeSum(keys)

                // Index to choose to traverse downwards
                lidx  = min i in [0, n-1] such that a.byteOffset < S[i+1]

                // Number of elements to the left of this index
                nleft = S[lidx]

                // Subtract the number of elements we skipped from the current byte offset
                byteOffset -= nleft

                // Fetch the child page at the chosen index
                nextPage = fetchPage(getLeafPointer(page, lidx))

                // Optionally save the stack
                IF saveStack:
                    push saveStack <- { page, lidx }
                ELSE:
                    release(page)

                // Descend into child
                page = nextPage

            CASE "Leaf / Data List":
                return page, min(byteOffset, page.usedCount)

            DEFAULT:
                UNREACHABLE
```

Write
-----

Write takes a byte offset, an element size, a stride, and a count. It seeks to 
the starting position in the leaf chain, upgrades the page to writable, then 
enters a loop alternating between two phases. In the ACTIVE phase it stamps 
bytes from the caller's source stream directly into the page at the current index, 
advancing until one element's worth of bytes has been written. It then enters 
the SKIPPING phase, where it advances the index by (stride-1) elements without 
touching those bytes, leaving them unchanged. When a page is exhausted the writer 
follows the next-page pointer and continues. The loop terminates when the byte 
limit is reached, the source is exhausted, or the chain ends. Size changes 
are propagated up the tree as described in section SEC.

```
FUNCTION WriteForward(root, size, stride, nelem, byteOffset):

    // Initialize a seek - we don't need to save the page stack
    (page, lidx) = Seek(byteOffset, NULL)

    // Upgrade this page to writable
    makeWritable(page)

    // Initialize write state
    total_bwrite = 0              // Total bytes written
    max_bwrite   = size * nelem   // Maximum bytes to write
    bnext        = size           // Next write length
    state        = ACTIVE         // We are either writing (ACTIVE) or skipping (SKIPPING)

    WHILE total_bwrite < max_bwrite:

        // Minimum of the remainder of this page or bnext
        next_amount = min(page.usedCount - lidx, bnext)

        // Further restrict the number we can write if we are active
        IF state == ACTIVE:
            next_amount = min(next_amount, max_bwrite - total_bwrite)

        IF next_amount == 0:

            // End of page; move to next
            IF page.next == NULL:
                termination = DATA_EXHAUSTED
                GOTO done

            // Fetch the next page
            nextPage = fetchWritablePage(page.next)

            // Release this page
            release(page)

            // Reset local state
            lidx = 0
            page = nextPage

            // Recompute for new page
            next_amount = page.usedCount - lidx
            next_amount = min(next_amount, bnext)

            IF max_bwrite > 0 AND state == ACTIVE:
                next_amount = min(next_amount, max_bwrite - total_bwrite)

        SWITCH state:

            CASE ACTIVE:
                // Write bytes from source into page
                write        = readFromSrc(src, into page.data[lidx], next_amount)
                lidx         += write
                total_bwrite += write
                bnext        -= write

                IF bnext == 0:
                    // Element done; enter skip phase
                    bnext = (stride - 1) * size
                    state = SKIPPING

                    IF bnext == 0:
                        // stride == 1; skip phase is empty, stay active
                        bnext = size
                        state = ACTIVE

            CASE SKIPPING:
                // Advance past bytes without writing
                lidx  += next_amount
                bnext -= next_amount

                IF bnext == 0:
                    // Skip done; write next element
                    bnext = size
                    state = ACTIVE

        IF src.isDone:
            termination = SRC_DONE_WRITING
            BREAK

done:
    release(page)

    RETURN total_bwrite / size
```

Read
----

Read takes a byte offset, an element size, a stride, and a count. It seeks to 
the starting position in the leaf chain, then enters a loop alternating between 
two phases. In the ACTIVE phase it copies bytes from the current page into the 
caller's destination stream, advancing the local index until one element's 
worth of bytes has been read. It then enters the SKIPPING phase, where it advances 
the index by (stride-1) elements without copying them, leaving a gap in the 
output corresponding to the strided elements. When a page is exhausted the reader 
follows the next-page pointer and continues. The loop terminates when the byte 
limit is reached, the destination is exhausted, or the chain ends.

```
FUNCTION ReadForward(root, size, stride, nelem, byteOffset):
    // Initialize a seek - we don't need to save the page stack
    (page, lidx) = Seek(byteOffset, NULL)
    // Initialize read state
    total_bread = 0              // Total bytes read
    max_bread   = size * nelem   // Maximum bytes to read
    bnext       = size           // Next read length
    state       = ACTIVE         // We are either reading (ACTIVE) or skipping (SKIPPING)

    WHILE total_bread < max_bread:
        // Minimum of the remainder of this page or bnext
        next_amount = min(page.usedCount - lidx, bnext)
        // Further restrict the number we can read if we are active
        IF state == ACTIVE:
            next_amount = min(next_amount, max_bread - total_bread)

        IF next_amount == 0:
            // End of page; move to next
            IF page.next == NULL:
                termination = DATA_EXHAUSTED
                GOTO done
            // Fetch the next page
            nextPage = fetchPage(page.next)
            // Release this page
            release(page)
            // Reset local state
            lidx = 0
            page = nextPage
            // Recompute for new page
            next_amount = min(page.usedCount - lidx, bnext)
            IF state == ACTIVE:
                next_amount = min(next_amount, max_bread - total_bread)

        SWITCH state:
            CASE ACTIVE:
                // Read bytes from page into destination
                read        = writeIntoDest(dest, from page.data[lidx], next_amount)
                lidx        += read
                total_bread += read
                bnext       -= read

                IF bnext == 0:
                    // Element done; enter skip phase
                    bnext = (stride - 1) * size
                    state = SKIPPING
                    IF bnext == 0:
                        // stride == 1; skip phase is empty, stay active
                        bnext = size
                        state = ACTIVE

            CASE SKIPPING:
                // Advance past bytes without reading
                lidx  += next_amount
                bnext -= next_amount

                IF bnext == 0:
                    // Skip done; read next element
                    bnext = size
                    state = ACTIVE

        IF dest.isDone:
            termination = DEST_DONE_READING
            BREAK

done:
    release(page)
    RETURN total_bread / size 
```

Insert
------

Insert takes a byte offset and a source stream. It seeks to the insertion point 
within a leaf page, then splits that page at the local index by reading the tail 
bytes into a temporary buffer. It then streams new bytes from the source into the 
page chain, allocating new pages as each fills up. Once the source is exhausted, 
the displaced tail is re-appended to the end of the written region, again spilling 
onto new pages if needed. Finally the chain is re-linked to whatever followed the 
original page. Structural changes to the leaf level are propagated up the tree as 
described in section SEC.

```
FUNCTION Insert(root, size, nelem, byteOffset, src):
    total_to_write = size * nelem

    // Empty tree; allocate the first page
    IF root == NULL:
        page = allocatePage()
        root = page
        lidx = 0
    ELSE:
        // Seek to insertion point, saving the page stack for rebalancing
        (page, lidx) = Seek(byteOffset, saveStack=true)
        makeWritable(page)

    // Split: save the tail of the current page from lidx onward
    tail     = page.data[lidx:]
    tailLen  = len(tail)
    page.truncate(lidx)

    // Remember what comes after this page before we start inserting
    savedNext = page.next

    total_written = 0

    // Phase 1: stream new bytes into the page chain
    WHILE nelem == 0 OR total_written < total_to_write:
        avail = page.capacity - page.usedCount

        IF avail == 0:
            // Page full; link a new one and continue
            nextPage = allocatePage()
            link(page, nextPage)
            commitPageToUpdateLog(page)
            release(page)
            page = nextPage
            avail = page.capacity

        next_amount = avail
        IF nelem != 0:
            next_amount = min(avail, total_to_write - total_written)

        written = readFromSrc(src, into page.data[page.usedCount], next_amount)

        IF written == 0 AND src.isDone:
            BREAK

        page.usedCount += written
        total_written  += written

    // Phase 2: re-append the displaced tail
    tailOffset = 0
    WHILE tailOffset < tailLen:
        written = appendToPage(page, tail[tailOffset:])
        tailOffset += written

        IF page.isFull AND tailOffset < tailLen:
            // Tail spills onto another new page
            nextPage = allocatePage()
            link(page, nextPage)
            commitPageToUpdateLog(page)
            release(page)
            page = nextPage

    // Re-link to whatever followed the original page
    IF savedNext != NULL AND savedNext != page.next:
        nextPage = fetchWritablePage(savedNext)
        link(page, nextPage)

    // Balance the leaf and propagate size changes up the tree
    root = Rebalance(page, pageStack)

    RETURN total_written 
```

Remove
------

Remove takes a byte offset, an element size, a stride, and a count. It seeks 
to the starting position then compacts the leaf chain in place using two cursors: 
a writer and a reader. Initially they share the same page. The reader advances 
through the chain in two alternating phases — in the REMOVING phase it skips 
over one element's worth of bytes without copying them, optionally streaming 
them out to the caller; in the SKIPPING phase it copies (stride-1) elements' 
worth of bytes from the reader position into the writer position, closing the 
gap left by the removed elements. When the reader exhausts a page that has 
separated from the writer, that page is deleted and the chain is re-linked. 
Once all targeted elements are removed, any remaining reader data is drained 
into the writer pages and all spent reader pages are deleted. Size changes 
are propagated up the tree as described in section SEC.

```
FUNCTION Remove(root, size, stride, nelem, byteOffset, dest):
    IF root == NULL:
        RETURN 0

    // Seek to removal point, saving the page stack for rebalancing
    (writer, write_idx) = Seek(byteOffset, saveStack=true)
    makeWritable(writer)

    reader    = NULL       // second cursor; NULL means reader == writer
    read_idx  = write_idx
    total_removed = 0
    max_remove    = size * nelem
    bnext         = size
    phase         = REMOVING

    // Helper: current read page is reader if open, else writer
    curReader = () => reader ?? writer

    // Phase 1: Remove / Skip

    WHILE max_remove == 0 OR total_removed < max_remove:

        sro  = curReader()
        rlen = sro.usedCount

        SWITCH phase:

            CASE REMOVING:
                next_amount = min(bnext, rlen - read_idx)
                IF max_remove > 0:
                    next_amount = min(next_amount, max_remove - total_removed)

                IF next_amount == 0:
                    // End of reader page; advance to next
                    (reader, read_idx, isEOF) = AdvanceReader(writer, reader, write_idx)
                    IF isEOF: GOTO drain
                    CONTINUE

                // Optionally stream removed bytes out to caller
                IF dest != NULL:
                    writeIntoDest(dest, from sro.data[read_idx], next_amount)

                read_idx      += next_amount
                total_removed += next_amount
                bnext         -= next_amount

                IF bnext == 0:
                    // One element removed; enter skip phase
                    bnext = size * (stride - 1)
                    IF bnext > 0:
                        phase = SKIPPING
                    ELSE:
                        bnext = size    // stride == 1; stay REMOVING

                IF max_remove > 0 AND total_removed == max_remove:
                    GOTO drain

            CASE SKIPPING:
                next_amount = min(bnext, rlen - read_idx)
                next_amount = min(next_amount, writer.capacity - write_idx)

                IF next_amount == 0:
                    IF read_idx == rlen:
                        // End of reader page; advance reader
                        (reader, read_idx, isEOF) = AdvanceReader(writer, reader, write_idx)
                        IF isEOF: GOTO drain
                    ELSE IF write_idx == writer.capacity:
                        // Writer page full; flush and advance
                        (writer, write_idx) = AdvanceWriter(writer, reader, write_idx)
                    CONTINUE

                // Copy surviving bytes from reader into writer
                memmove(writer.data[write_idx], sro.data[read_idx], next_amount)
                write_idx += next_amount
                read_idx  += next_amount
                bnext     -= next_amount

                IF bnext == 0:
                    // Skip window done; remove next element
                    bnext = size
                    phase = REMOVING

        IF dest != NULL AND dest.isDone:
            GOTO drain

drain:
    // Phase 2: Copy all remaining reader data into writer, deleting spent pages

    WHILE true:
        sro         = curReader()
        rlen        = sro.usedCount
        next_amount = min(writer.capacity - write_idx, rlen - read_idx)

        IF next_amount == 0:
            IF read_idx == rlen:
                writer.usedCount = write_idx

                IF reader != NULL:
                    // Delete exhausted reader page and advance
                    nextPageNo = reader.next
                    deleteAndRelease(reader)
                    link(writer, fetchWritablePage(nextPageNo))
                    reader   = fetchWritablePage(nextPageNo)
                    read_idx = 0
                    IF reader == NULL: BREAK
                    CONTINUE
                ELSE:
                    BREAK
            ELSE IF write_idx >= writer.capacity:
                (writer, write_idx) = AdvanceWriter(writer, reader, write_idx)
                CONTINUE

        memmove(writer.data[write_idx], sro.data[read_idx], next_amount)
        write_idx += next_amount
        read_idx  += next_amount

    // Phase 3: Balance and propagate size changes up the tree
    root = Rebalance(writer, pageStack)

    RETURN total_removed / size 
```

Rebalance
---------

```
FUNCTION ExecuteRight(params):
    LOOP:
        IF input has no more right entries to observe:
            params.lidx += appendRight(input, cur, params.lidx)

            IF input not fully consumed right:
                // cur is full; commit, splice in a new page, continue
                commit(output, cur)
                newPage = allocateInnerPage()
                link(cur, newPage, limit)
                release(cur)
                cur         = newPage
                params.lidx = 0
                CONTINUE
            ELSE:
                cur.keyCount = params.lidx
                RETURN RightToLeft(params)
        ELSE:
            observeAllRight(input, limit)
            params.lidx += appendRight(input, cur, params.lidx)

            IF input not done AND params.lidx > PAGE_HALF:
                // Shift right into limit slot
                IF limit == NULL:
                    limit = allocateInnerPage()
                    link(cur, limit)
                ELSE:
                    makeWritable(limit)
                cur.keyCount = params.lidx
                commit(output, cur)
                release(cur)
                cur         = limit
                limit       = cur.next
                params.lidx = 0
            ELSE:
                // Merge: delete limit, re-link around it
                IF limit != NULL:
                    nextNext = limit.next
                    deleteAndRelease(limit)
                    link(cur, nextNext)
                    limit = nextNext
                    recordDeletion(output, cur, deletedPg)


FUNCTION ExecuteLeft(params):
    LOOP:
        IF input has no more left entries to observe:
            params.lidx -= appendLeft(input, cur, params.lidx)

            IF input not fully consumed left:
                // cur is full; commit, splice in a new page to the left, continue
                commit(output, cur)
                newPage = allocateInnerPage()
                link(limit, newPage, cur)
                release(cur)
                cur         = newPage
                params.lidx = PAGE_MAX
                CONTINUE
            ELSE:
                cutLeft(cur, params.lidx)
                params.lidx = cur.keyCount
                RETURN LeftToRight(params)
        ELSE:
            observeAllLeft(input, limit)
            params.lidx -= appendLeft(input, cur, params.lidx)

            IF input not done AND (PAGE_MAX - params.lidx) > PAGE_HALF:
                // Shift left into limit slot
                IF limit == NULL:
                    limit = allocateInnerPage()
                    link(limit, cur)
                ELSE:
                    makeWritable(limit)
                cutLeft(cur, params.lidx)
                commit(output, cur)
                release(cur)
                cur         = limit
                limit       = cur.prev
                params.lidx = PAGE_MAX
            ELSE:
                // Merge: delete limit, re-link around it
                IF limit != NULL:
                    prevPrev = limit.prev
                    deleteAndRelease(limit)
                    link(prevPrev, cur)
                    limit = prevPrev
                    recordDeletion(output, cur, deletedPg)


FUNCTION ApplyToPivot(params):
    observePivot(input, cur, params.lidx)

    // Fill cur maximally right then left
    params.lidx = PAGE_MAX - appendRightThenLeft(input, cur)

    IF input done left:
        cutLeft(cur, params.lidx)
        params.lidx = cur.keyCount

        IF input done right:
            // Fully resolved; balance and move up
            BalanceAndRelease(cur)
            RETURN MoveUpStack(params)

        // Still have right updates; open limit to the right
        cur.keyCount = PAGE_MAX
        IF cur.next != NULL:
            params.limit = fetchPage(cur.next)
        RETURN

    // Still have left updates; open limit to the left
    IF cur.prev != NULL:
        params.limit = fetchPage(cur.prev)


FUNCTION MoveUpStack(params):
    IF layer is new root:
        // All levels above are now obsolete; delete them
        WHILE stack not empty:
            PopStack(params)
            DeleteChain(cur)
        params.root = layer_root
        RETURN

    IF stack is empty:
        // Tree grows by one level
        cur         = allocateInnerPage()
        params.root = cur.pageNo
        params.lidx = 0
    ELSE:
        PopStack(params)
        makeWritable(cur)

    // Swap update buffers and apply to the next level up
    swap(params.input, params.output)
    resetOutput(params.output, cur)
    RETURN ApplyToPivot(params)

FUNCTION Rebalance(params):
    LOOP:
        MoveUpStack(params)

        IF layer is root:
            RETURN

        done = true

        IF input has left updates:
            done = false
            ExecuteLeft(params)

        IF input has right updates:
            done = false
            ExecuteRight(params)

        IF done:
            RETURN
```

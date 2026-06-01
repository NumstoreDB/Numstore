Swarm Testing 
=============

Available Operations 
--------------------

The available things you can do to a numstore database are:
*  **Create**       a new variable
*  **Delete**       a variable variable
*  **Get**          a variable 
*  **Insert**       data into a variable
*  **Read**         data from a variable
*  **Write**        data into a variable
*  **Remove**       data from a variable
*  **Begin**        a transation
*  **Commit**       an open transaction
*  **Rollback**     an open transaction
*  **Close**        and reopen a database
*  **Crash**        and reopen a database

These can be encoded using numbers:
*  **Create**:      0 
*  **Delete**:      1 
*  **Get**:         2 
*  **Insert**:      3 
*  **Read**:        4 
*  **Write**:       5 
*  **Remove**:      6 
*  **Begin**:       7 
*  **Commit**:      8 
*  **Rollback**:    9 
*  **Close**:       10
*  **Crash**:       11

A swarm test has a given domain of available operations. 

For example, the simple path is 3.4.5.6 - e.g. which is 
any modification to an array (the heavy hitter operations)

In a swarm test, we randomly execute a choice from the domain 
of available options over and over in a loop until we close 
the application. 

Also, the "swarm" aspect of the testing means every once in a 
while, we disable an operation. So we might disable all removes,
simulating a high modification workload. This sets our space 
of available options to 3.4.5. 

The encoding per file is which operations are globally allowed. 
So 3-4-5-6 means insert read write and remove operations are 
globally allowed, with the added effect of enabling

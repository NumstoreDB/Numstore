Usage
=====

How to link against and call lib1.

Linking
-------

Add ``-llib1`` to your link flags::

   cc myapp.c -llib1 -o myapp

Quick example
-------------

.. code-block:: c

   #include <lib1.h>

   int main(void) {
       lib1_init();
       return 0;
   }

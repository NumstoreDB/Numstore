1. Move the os directory to the core/os - change includes
2. Fix any bugs in the os layer.
3. Change all invokations of i_xyz to the explicit thing.table->(thing.self, ...)
4. Lots of refactoring of method calls - like file close needs to pass in the os 
   and memory uses i_mem, i_filesystem doesn't exist anymore, closest cousin is 
   i_os
5. Get it compiling and get the simtest using faulty os - doesn't have to run 
   I'll fix bugs of course, but it should compile and run just doesn't need 
   to pass necessarily.
6. Take a scan through the windows layer, even though you can't compile it, get it 
   up to shape the best you can

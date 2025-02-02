:i count 4
:b shell 56
./bin/lewc examples/rule110.lew -o ./int/tests/rule110.s
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 49
as ./int/tests/rule110.s -o ./int/tests/rule110.o
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 56
gcc -static ./int/tests/rule110.o -o ./bin/tests/rule110
:i returncode 0
:b stdout 0

:b stderr 0

:b shell 19
./bin/tests/rule110
:i returncode 0
:b stdout 868
                            * 
                           ** 
                          *** 
                         ** * 
                        ***** 
                       **   * 
                      ***  ** 
                     ** * *** 
                    ******* * 
                   **     *** 
                  ***    ** * 
                 ** *   ***** 
                *****  **   * 
               **   * ***  ** 
              ***  **** * *** 
             ** * **  ***** * 
            ******** **   *** 
           **      ****  ** * 
          ***     **  * ***** 
         ** *    *** ****   * 
        *****   ** ***  *  ** 
       **   *  ***** * ** *** 
      ***  ** **   ******** * 
     ** * ******  **      *** 
    *******    * ***     ** * 
   **     *   **** *    ***** 
  ***    **  **  ***   **   * 
 ** *   *** *** ** *  ***  ** 

:b stderr 0


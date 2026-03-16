class A [
 fields x:int 
 
 method m() returning int with locals tmp:int:
  tmp = &this.x
  return tmp
]

class B [
 fields y:A 
 
 method m() returning int with locals:
  return 0 
]

main with x:A:
 x = null:A
 x = @A
 !x.x = 3
 print(^x.m())

class nothing [
    fields x:int, y:int, z:int
    method getx() returning int with locals:
        return &this.x
    method gety() returning int with locals:
        return &this.y
    method getz() returning int with locals:
        return &this.z
]

class RubeGoldberg [
    fields n:nothing
    method setn(x:int, y:int, z:int) returning int with locals:
        !this.n = @nothing
        
        !&this.n.x = x
        !&this.n.y = y
        !&this.n.z = z
        
        return ^&this.n.getx()
]
main with x:int, rb:RubeGoldberg:
    x = 10
    while (x > 0): {
        rb = @RubeGoldberg
        _ = ^rb.setn((x / 100), (x - 1500), (x * 2))
        x = (x - 1)
    }
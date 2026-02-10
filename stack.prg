class ListNode [
    fields val, next
    method getNext() with locals:
        return &this.next
    method getVal() with locals:
        return &this.val
]
class Stack [
    fields list
    method push(v) with locals tmp:
        tmp = @ListNode
	!tmp.val = v
	!tmp.next = &this.list
	!this.list = tmp
	return 0
    method pop() with locals tmp, head:
        if (&this.list == 0): {
            return 0
        } else {
            head = &this.list
            tmp = ^head.getVal()
            !this.list = ^head.getNext()
            return tmp
        }
]
class Stacker [
    fields
    method do(stk) with locals x, v:
        x = 20
        while (x > 0): {
            _ = ^stk.push(x)
            x = (x - 1)
        }
        v = ^stk.pop()
        while (v != 0): {
            print(v)
            v = ^stk.pop()
        }
]
main with stk, stkr:
    stk = @Stack
    !stk.list = 0
    stkr = @Stacker
    _ = ^stkr.do(stk)
import sys

def rshift(val, n): return val >> n if val >= 0 else (val + (1 << 64)) >> n

def get_set_and_tag(address, tbits, sbits):
    tag_mask = ~0 << (64 - tbits)
    set_mask = ~0 << (64 - sbits)
    block_offset_mask = rshift(~0, tbits + sbits)

    set_mask = rshift(set_mask, tbits)

    tag = (tag_mask & address) >> (64 - tbits)
    set = (set_mask & address) >> ((64 - tbits) - sbits)
    block_offset = (block_offset_mask & address)

    return [tag, set, block_offset]

def main():
    sets = []

    rows = 0
    cols = 0

    if(len(sys.argv) < 3):
        rows = int(input("Rows: "))
        cols = int(input("Cols: "))
    else:
        rows = int(sys.argv[1])
        cols = int(sys.argv[2])

    for r in range(rows):
        for c in range(cols):
            print("{:2d}".format(get_set_and_tag(c*4 + (cols*4)*r, 54, 5)[1]), end = " ")
            if((c + 1) % 8 == 0):
                print("  ", end = " ")
        print("")
        if((r + 1) % 8 == 0):
            print("")

    print("")
    print("")

    for r in range(rows):
        for c in range(cols):
            print("{:2d}".format(get_set_and_tag(c*4 + (cols*4)*r, 54, 5)[2]), end = " ")
            if((c + 1) % 8 == 0):
                print("  ", end = " ")
        print("")
        if((r + 1) % 8 == 0):
            print("")

    '''while True:
        r = int(input("Row: "), 16)
        c = int(input("Col: "), 16)

        tag, set = get_set_and_tag(c*4 + 256*r, 54, 5)
        print("Tag: " + str(tag))
        print("Set: " + str(set))'''

    #print(get_set_and_tag(18378908604322283520, 8, 8))

if __name__ == "__main__":
    main()

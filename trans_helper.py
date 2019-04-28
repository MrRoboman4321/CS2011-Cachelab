import sys

def rshift(val, n): return val >> n if val >= 0 else (val + (1 << 64)) >> n

def is_transpose(A, B):
    for row in range(len(A)):
        for col in range(len(A[0])):
            if(A[row][col] != B[col][row]):
                return False
    return True

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

    A = []
    B = []

    for r in range(rows):
        A.append([])
        for c in range(cols):
            A[-1].append(c + cols*r)

    for c in range(cols):
        B.append([])
        for r in range(rows):
            B[-1].append(0)

    print("A ARRAY")
    for r in range(rows):
        for c in range(cols):
            print("{:4d}".format(A[r][c]), end = " ")
            if((c + 1) % 8 == 0):
                print("  ", end = " ")
        print("")
        if((r + 1) % 8 == 0):
            print("")

    print("")
    print("")
    '''
    for block_row in range(16):
        for block_col in range(8):
            for row in range(4):
                for col in range(8):
                    B[4*block_col + col][4*block_row + row] = A[4*block_row + row][8*block_col + col]
    '''

    block_rows = int((rows - (rows % 8))/8);
    block_cols = int((cols - (cols % 8))/8);

    print(f"block rows: {block_rows}")
    print(f"block cols: {block_cols}")

    for block_row in range(block_rows):
        for block_col in range(block_cols):
            for row in range(8):
                for col in range(8):
                    B[8*block_col + col][8*block_row + row] = A[8*block_row + row][8*block_col + col]
    for row in range(rows):
        for col in range(block_cols * 8, cols):
            B[col][row] = A[row][col]
    for row in range(block_rows * 8, rows):
        for col in range(0, block_cols * 8):
            B[col][row] = A[row][col]

    print("TRANSPOSED B")
    for r in range(cols):
        for c in range(rows):
            print("{:4d}".format(B[r][c]), end = " ")
            if((c + 1) % 8 == 0):
                print("  ", end = " ")
        print("")
        if((r + 1) % 8 == 0):
            print("")

    if is_transpose(A, B):
        print("Transpose successful")
    else:
        print("Transpose failed")

    print("")
    print("")

    print("TAGS")
    for r in range(rows):
        for c in range(cols):
            print("{:2d}".format(get_set_and_tag(c*4 + (cols*4)*r, 54, 5)[0]), end = " ")
            if((c + 1) % 8 == 0):
                print("  ", end = " ")
        print("")
        if((r + 1) % 8 == 0):
            print("")

    print("")
    print("")

    print("SET INDEXES")
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

    print("BLOCK OFFSETS")
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

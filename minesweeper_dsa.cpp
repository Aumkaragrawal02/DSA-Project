// =============================================================================
//  MINESWEEPER — DSA VISUALIZER
//  Full C++ implementation with detailed DSA concept annotations
//
//  DSA CONCEPTS USED IN THIS PROJECT:
//  ─────────────────────────────────────────────────────────────────────────────
//  1. 2D ARRAY   — The entire game board is stored as a 2D array (grid).
//                  Every cell is accessed by G[row][col].
//
//  2. STRUCT     — Each cell on the board is a C++ struct holding 4 fields:
//                  mine (bool), revealed (bool), flagged (bool), adj (int).
//
//  3. QUEUE      — BFS (flood-fill reveal) uses a std::queue to process cells
//                  in FIFO order — first enqueued cell is first to be visited.
//
//  4. HASH SET   — std::unordered_set<int> tracks which cells have already
//                  been visited, giving O(1) lookup to prevent revisiting.
//
//  5. BFS        — Breadth-First Search is the algorithm used to flood-fill
//                  reveal all connected empty cells when a blank tile is clicked.
//
//  NOTE: This is a console/logic version. The HTML game visualizes these exact
//        same algorithms live on the board tiles as you play.
// =============================================================================

#include <iostream>
#include <vector>
#include <queue>               // ← DSA: QUEUE  — used in bfsReveal()
#include <unordered_set>       // ← DSA: HASH SET — used in bfsReveal()
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>
#include <algorithm>

using namespace std;

// =============================================================================
//  DSA CONCEPT 1: STRUCT — Cell data structure
//  ─────────────────────────────────────────────────────────────────────────────
//  Every tile on the board is represented as a Cell struct.
//  In the HTML visualizer, hovering a tile shows these exact 4 fields live
//  in the "2D ARRAY" panel on the right side.
//
//  Fields:
//    mine     — true if this cell contains a mine
//    revealed — true if the player has uncovered this cell
//    flagged  — true if the player has placed a flag here
//    adj      — count of mines in the 8 neighboring cells (adjacency count)
// =============================================================================
struct Cell {
    bool mine     = false;   // Is there a mine here?
    bool revealed = false;   // Has the player revealed this cell?
    bool flagged  = false;   // Has the player flagged this cell?
    int  adj      = 0;       // Number of adjacent mines (0–8)
};

// =============================================================================
//  DSA CONCEPT 2: 2D ARRAY — The game board
//  ─────────────────────────────────────────────────────────────────────────────
//  The board is a 2D vector of Cell structs: G[rows][cols].
//  Every position is accessed by G[r][c] — row index, then column index.
//
//  In the HTML visualizer, the "2D ARRAY" panel shows:
//    • G[r][c] — the coordinates of the cell you're hovering
//    • idx = r * cols + c — the flat 1D index (used as hash key)
//    • A 3×3 neighbor grid showing which of the 8 neighbors are in-bounds
//
//  Neighbor access pattern (dr, dc offsets):
//    (-1,-1) (-1, 0) (-1,+1)
//    ( 0,-1) [CELL]  ( 0,+1)
//    (+1,-1) (+1, 0) (+1,+1)
// =============================================================================
const int DR[] = {-1,-1,-1, 0, 0,+1,+1,+1};  // Row offsets for 8 neighbors
const int DC[] = {-1, 0,+1,-1,+1,-1, 0,+1};  // Col offsets for 8 neighbors

// Difficulty configurations
struct Config {
    int rows, cols, mines, timeLimit;
    string name;
};

const Config CFGS[] = {
    { 9,  9,  10, 180,  "EASY"   },   // 9×9  grid, 10  mines, 3  min
    {16, 16,  40, 600,  "MEDIUM" },   // 16×16 grid, 40  mines, 10 min
    {16, 30,  99, 1200, "HARD"   },   // 16×30 grid, 99  mines, 20 min
};

// =============================================================================
//  BOARD CLASS — wraps all game logic and DSA data structures
// =============================================================================
class Minesweeper {
public:
    // ── DSA: 2D ARRAY — board grid ──────────────────────────────────────────
    vector<vector<Cell>> G;     // G[r][c] — the 2D board array

    int rows, cols, mines;
    bool gameOver   = false;
    bool firstClick = true;
    int  flags      = 0;
    int  hintsLeft  = 3;

    // Counters for the BFS stats panel in the HTML visualizer
    int totalBfsSteps   = 0;
    int totalEnqueued   = 0;
    int totalDequeued   = 0;
    int totalRevealed   = 0;

    // ─────────────────────────────────────────────────────────────────────────
    //  Constructor — initializes the 2D array of Cell structs
    // ─────────────────────────────────────────────────────────────────────────
    Minesweeper(int r, int c, int m)
        : rows(r), cols(c), mines(m)
    {
        // DSA: 2D ARRAY — allocate rows × cols grid, each cell default-initialized
        G.assign(rows, vector<Cell>(cols));
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  inBounds — bounds check before any G[r][c] access
    //  This is the "out of bounds" check shown in the 2D ARRAY neighbor grid
    // ─────────────────────────────────────────────────────────────────────────
    bool inBounds(int r, int c) const {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  flatKey — converts (r, c) to a single integer for the hash set
    //  DSA: HASH SET — key formula: r * cols + c
    //  In the HTML visualizer, the Hash Set panel shows this exact formula
    //  and which bucket (key % 10) the cell lands in.
    // ─────────────────────────────────────────────────────────────────────────
    int flatKey(int r, int c) const {
        // DSA: HASH SET key — maps 2D coordinate to unique 1D integer
        return r * cols + c;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  reset — clears the board and reinitializes for a new game
    // ─────────────────────────────────────────────────────────────────────────
    void reset() {
        // DSA: 2D ARRAY — re-initialize all cells in the grid
        G.assign(rows, vector<Cell>(cols));
        gameOver   = false;
        firstClick = true;
        flags      = 0;
        hintsLeft  = 3;
        totalBfsSteps = totalEnqueued = totalDequeued = totalRevealed = 0;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  placeMines — randomly places mines AFTER the first click
    //  The 3×3 zone around the first click is guaranteed mine-free (safe start)
    // ─────────────────────────────────────────────────────────────────────────
    void placeMines(int safeR, int safeC) {
        // DSA: HASH SET — safeZone ensures O(1) lookup for safe cells
        unordered_set<int> safeZone;
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++)
                if (inBounds(safeR + dr, safeC + dc))
                    safeZone.insert(flatKey(safeR + dr, safeC + dc));

        int placed = 0;
        srand((unsigned)time(nullptr));

        while (placed < mines) {
            int r = rand() % rows;
            int c = rand() % cols;
            // DSA: HASH SET — O(1) lookup to skip safe zone and already-mined cells
            if (!G[r][c].mine && safeZone.find(flatKey(r, c)) == safeZone.end()) {
                G[r][c].mine = true;
                placed++;
            }
        }

        // After placing mines, compute adj count for every non-mine cell
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                if (!G[r][c].mine)
                    G[r][c].adj = countAdjacentMines(r, c);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  countAdjacentMines — counts mines in all 8 neighbors of G[r][c]
    //  DSA: 2D ARRAY — uses the DR/DC offset arrays to walk neighbors
    //  This is what fills the "adj" field of the Cell struct
    // ─────────────────────────────────────────────────────────────────────────
    int countAdjacentMines(int r, int c) const {
        int count = 0;
        // DSA: 2D ARRAY — iterate over 8 neighbors using offset arrays
        for (int i = 0; i < 8; i++) {
            int nr = r + DR[i];
            int nc = c + DC[i];
            if (inBounds(nr, nc) && G[nr][nc].mine)
                count++;
        }
        return count;
    }

    // =========================================================================
    //  DSA CONCEPT 3+4+5: BFS FLOOD-FILL using QUEUE + HASH SET
    //  ─────────────────────────────────────────────────────────────────────────
    //  bfsReveal — the core algorithm that runs every time the player
    //              double-clicks a non-mine cell.
    //
    //  ALGORITHM WALKTHROUGH:
    //  ──────────────────────
    //  1. Start: push the clicked cell (sr, sc) into the queue.
    //            Mark it as visited in the hash set.
    //
    //  2. Loop while queue is not empty:
    //     a. Dequeue the FRONT cell [r, c]  ← QUEUE FIFO
    //     b. Mark G[r][c].revealed = true   ← 2D ARRAY write
    //     c. If G[r][c].adj == 0 (blank cell):
    //        → Loop over all 8 neighbors via DR/DC offsets  ← 2D ARRAY
    //        → For each neighbor [nr][nc]:
    //          · Compute key = nr * cols + nc               ← HASH SET key
    //          · If key NOT in visited set:                 ← HASH SET O(1)
    //            - Add key to visited set                   ← HASH SET insert
    //            - Enqueue [nr, nc]                        ← QUEUE push
    //
    //  3. End: all reachable connected blank cells (and their numbered borders)
    //     are now revealed in a single BFS sweep.
    //
    //  TIME COMPLEXITY:  O(V + E) where V = cells, E = edges (adjacencies)
    //  SPACE COMPLEXITY: O(V) for the queue and hash set
    //
    //  WHY BFS not DFS?
    //  BFS expands ring by ring from the click point — this matches the
    //  visual "flood ripple" effect you see on the board. DFS would zigzag
    //  unpredictably across the board instead of expanding outward evenly.
    // =========================================================================
    int bfsReveal(int sr, int sc) {
        // Guard: don't reveal flagged or already-revealed cells
        if (G[sr][sc].flagged || G[sr][sc].revealed) return 0;

        // ── DSA: QUEUE — FIFO structure for BFS ─────────────────────────────
        // std::queue uses a deque internally: push() appends to back,
        // front()/pop() reads and removes from front — pure FIFO.
        queue<pair<int,int>> bfsQueue;

        // ── DSA: HASH SET — tracks visited cells to avoid re-processing ─────
        // unordered_set gives O(1) average insert and lookup.
        // We use the flat key r*cols+c as the integer hash key.
        unordered_set<int> visited;

        // ── STEP 1: Enqueue source cell ──────────────────────────────────────
        bfsQueue.push({sr, sc});             // DSA: QUEUE — push to back
        visited.insert(flatKey(sr, sc));     // DSA: HASH SET — mark visited
        totalEnqueued++;

        int revealCount = 0;

        // ── STEP 2: BFS main loop ────────────────────────────────────────────
        while (!bfsQueue.empty()) {

            // DSA: QUEUE — dequeue from front (FIFO order)
            auto [r, c] = bfsQueue.front();
            bfsQueue.pop();
            totalDequeued++;
            totalBfsSteps++;

            // Skip flagged cells (player has marked them deliberately)
            if (G[r][c].flagged) continue;

            // DSA: 2D ARRAY — write revealed=true to G[r][c]
            G[r][c].revealed = true;
            revealCount++;
            totalRevealed++;

            // Only expand neighbors if this is a BLANK cell (adj == 0)
            // Numbered cells (adj > 0) are revealed but do NOT expand further
            if (G[r][c].adj == 0) {

                // DSA: 2D ARRAY — iterate all 8 neighbors using DR/DC offsets
                for (int i = 0; i < 8; i++) {
                    int nr = r + DR[i];
                    int nc = c + DC[i];

                    // DSA: 2D ARRAY — bounds check before accessing G[nr][nc]
                    if (!inBounds(nr, nc)) continue;

                    // DSA: HASH SET — O(1) check: has this cell been queued?
                    int key = flatKey(nr, nc);
                    if (visited.count(key)) continue;    // already seen

                    // Skip mines (they should never be revealed by flood-fill)
                    if (G[nr][nc].mine) continue;

                    // Skip flagged neighbors
                    if (G[nr][nc].flagged) continue;

                    // DSA: HASH SET — O(1) insert to mark as visited
                    visited.insert(key);

                    // DSA: QUEUE — enqueue neighbor for future processing
                    bfsQueue.push({nr, nc});
                    totalEnqueued++;
                }
            }
        }
        // ── BFS complete ─────────────────────────────────────────────────────

        return revealCount;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  handleClick — processes a player reveal action
    // ─────────────────────────────────────────────────────────────────────────
    bool handleClick(int r, int c) {
        if (gameOver || G[r][c].revealed || G[r][c].flagged) return false;

        // First click: place mines AFTER this click so first move is always safe
        if (firstClick) {
            firstClick = false;
            placeMines(r, c);
        }

        // Hit a mine — game over
        if (G[r][c].mine) {
            G[r][c].revealed = true;
            gameOver = true;
            revealAllMines();
            cout << "\n  *** BOOM! Mine at [" << r << "," << c << "] ***\n";
            return false;
        }

        // Safe cell — run BFS flood-fill
        // DSA: BFS — this is where all 4 DSA concepts come alive
        int revealed = bfsReveal(r, c);
        cout << "  BFS revealed " << revealed << " cells. "
             << "Queue ops: " << totalEnqueued + totalDequeued << "\n";

        return checkWin();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  toggleFlag — place or remove a flag on G[r][c]
    // ─────────────────────────────────────────────────────────────────────────
    void toggleFlag(int r, int c) {
        if (gameOver || G[r][c].revealed) return;
        // DSA: 2D ARRAY — direct write to G[r][c].flagged
        G[r][c].flagged = !G[r][c].flagged;
        flags += G[r][c].flagged ? 1 : -1;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  revealAllMines — called on game over to show all mine positions
    // ─────────────────────────────────────────────────────────────────────────
    void revealAllMines() {
        // DSA: 2D ARRAY — full traversal of the entire grid
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                if (G[r][c].mine)
                    G[r][c].revealed = true;
        gameOver = true;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  checkWin — scans the entire board to see if all safe cells are revealed
    // ─────────────────────────────────────────────────────────────────────────
    bool checkWin() const {
        int safeCells = rows * cols - mines;
        int revealed  = 0;
        // DSA: 2D ARRAY — O(rows × cols) scan of the entire grid
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                if (G[r][c].revealed) revealed++;
        return revealed == safeCells;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  giveHint — finds a safe unrevealed cell for the player
    //  Uses adjacency to prefer cells next to already-revealed tiles
    // ─────────────────────────────────────────────────────────────────────────
    pair<int,int> giveHint() const {
        // DSA: 2D ARRAY — scan all cells looking for a good candidate
        vector<pair<int,int>> candidates;
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                if (!G[r][c].revealed && !G[r][c].flagged && !G[r][c].mine)
                    candidates.push_back({r, c});

        if (candidates.empty()) return {-1, -1};

        // Prefer cells adjacent to revealed tiles
        for (auto& [r, c] : candidates) {
            for (int i = 0; i < 8; i++) {
                int nr = r + DR[i], nc = c + DC[i];
                // DSA: 2D ARRAY — neighbor check
                if (inBounds(nr, nc) && G[nr][nc].revealed)
                    return {r, c};
            }
        }
        return candidates[0];
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  printBoard — ASCII render of the board to the console
    // ─────────────────────────────────────────────────────────────────────────
    void printBoard(bool showAll = false) const {
        cout << "\n   ";
        for (int c = 0; c < cols; c++) cout << setw(3) << c;
        cout << "\n   ";
        for (int c = 0; c < cols; c++) cout << "---";
        cout << "\n";

        // DSA: 2D ARRAY — row-by-row traversal G[r][c]
        for (int r = 0; r < rows; r++) {
            cout << setw(2) << r << " |";
            for (int c = 0; c < cols; c++) {
                const Cell& cell = G[r][c];     // DSA: 2D ARRAY — G[r][c] access
                if (showAll && cell.mine) {
                    cout << "  *";
                } else if (!cell.revealed) {
                    cout << (cell.flagged ? "  F" : "  .");
                } else if (cell.mine) {
                    cout << "  *";
                } else if (cell.adj > 0) {
                    cout << setw(3) << cell.adj;
                } else {
                    cout << "   ";
                }
            }
            cout << "\n";
        }
        cout << "\n";
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  printDSAStats — prints all 4 DSA concept stats to console
    //  This mirrors what the HTML panels show in real time
    // ─────────────────────────────────────────────────────────────────────────
    void printDSAStats() const {
        cout << "\n  ╔══════════════════════════════════╗\n";
        cout << "  ║        DSA LIVE STATS            ║\n";
        cout << "  ╠══════════════════════════════════╣\n";
        cout << "  ║  [2D ARRAY]  Rows: " << setw(3) << rows
             << "  Cols: " << setw(3) << cols << "   ║\n";
        cout << "  ║  [STRUCT]    Cell fields: 4      ║\n";
        cout << "  ║              mine/rev/flag/adj   ║\n";
        cout << "  ╠══════════════════════════════════╣\n";
        cout << "  ║  [BFS]       Steps:    " << setw(6) << totalBfsSteps << "   ║\n";
        cout << "  ║  [QUEUE]     Enqueued: " << setw(6) << totalEnqueued << "   ║\n";
        cout << "  ║  [QUEUE]     Dequeued: " << setw(6) << totalDequeued << "   ║\n";
        cout << "  ║  [BFS]       Revealed: " << setw(6) << totalRevealed << "   ║\n";
        cout << "  ╠══════════════════════════════════╣\n";
        cout << "  ║  [HASH SET]  Visited cells tracked     ║\n";
        cout << "  ║              via O(1) hash lookup  ║\n";
        cout << "  ╚══════════════════════════════════╝\n\n";
    }
};

// =============================================================================
//  MAIN — interactive console game driver
// =============================================================================
int main() {
    cout << "\n";
    cout << "  ███╗   ███╗██╗███╗   ██╗███████╗███████╗██╗    ██╗███████╗███████╗██████╗\n";
    cout << "  ████╗ ████║██║████╗  ██║██╔════╝██╔════╝██║    ██║██╔════╝██╔════╝██╔══██╗\n";
    cout << "  ██╔████╔██║██║██╔██╗ ██║█████╗  ███████╗██║ █╗ ██║█████╗  █████╗  ██████╔╝\n";
    cout << "  ██║╚██╔╝██║██║██║╚██╗██║██╔══╝  ╚════██║██║███╗██║██╔══╝  ██╔══╝  ██╔═══╝\n";
    cout << "  ██║ ╚═╝ ██║██║██║ ╚████║███████╗███████║╚███╔███╔╝███████╗███████╗██║\n";
    cout << "  ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝ ╚══╝╚══╝ ╚══════╝╚══════╝╚═╝\n\n";
    cout << "  DSA Visualizer — C++ Edition\n";
    cout << "  Concepts: 2D Array · Struct · Queue · Hash Set · BFS\n\n";

    // Select difficulty
    cout << "  Select difficulty:\n";
    cout << "  [1] EASY   (9x9,   10 mines)\n";
    cout << "  [2] MEDIUM (16x16, 40 mines)\n";
    cout << "  [3] HARD   (16x30, 99 mines)\n";
    cout << "  > ";

    int choice = 1;
    cin >> choice;
    if (choice < 1 || choice > 3) choice = 1;

    const Config& cfg = CFGS[choice - 1];
    cout << "\n  Starting " << cfg.name << " — " << cfg.rows
         << "x" << cfg.cols << " grid, " << cfg.mines << " mines\n\n";

    // ── DSA: 2D ARRAY — create game board ───────────────────────────────────
    Minesweeper game(cfg.rows, cfg.cols, cfg.mines);

    cout << "  CONTROLS:\n";
    cout << "  r <row> <col>  — Reveal cell (triggers BFS)\n";
    cout << "  f <row> <col>  — Toggle flag\n";
    cout << "  h              — Get a hint\n";
    cout << "  s              — Show DSA stats\n";
    cout << "  q              — Quit\n\n";

    game.printBoard();

    string cmd;
    while (!game.gameOver) {
        cout << "  Flags: " << game.flags
             << "  |  BFS Steps so far: " << game.totalBfsSteps << "\n";
        cout << "  > ";
        cin >> cmd;

        if (cmd == "q") {
            cout << "  Goodbye!\n";
            break;

        } else if (cmd == "s") {
            // Print all 4 DSA stats
            game.printDSAStats();

        } else if (cmd == "h") {
            if (game.firstClick) {
                cout << "  Reveal a tile first!\n";
                continue;
            }
            auto [hr, hc] = game.giveHint();
            if (hr == -1) cout << "  No safe tiles left — you can win!\n";
            else cout << "  Hint: cell [" << hr << "," << hc << "] is safe\n";

        } else if (cmd == "r") {
            int r, c;
            cin >> r >> c;
            if (!game.inBounds(r, c)) {
                cout << "  Out of bounds!\n";
                continue;
            }
            // DSA: This triggers BFS flood-fill — all 4 concepts activate here
            bool won = game.handleClick(r, c);
            game.printBoard();
            if (won) {
                cout << "  *** YOU WIN! All safe cells revealed! ***\n";
                game.printDSAStats();
                break;
            }
            if (game.gameOver) {
                cout << "  GAME OVER — Mine revealed.\n";
                game.printBoard(true);
                game.printDSAStats();
                break;
            }

        } else if (cmd == "f") {
            int r, c;
            cin >> r >> c;
            if (!game.inBounds(r, c)) {
                cout << "  Out of bounds!\n";
                continue;
            }
            // DSA: 2D ARRAY — toggle G[r][c].flagged
            game.toggleFlag(r, c);
            game.printBoard();

        } else {
            cout << "  Unknown command. Use r/f/h/s/q\n";
        }
    }

    return 0;
}

// =============================================================================
//  COMPILATION INSTRUCTIONS
//  ─────────────────────────────────────────────────────────────────────────────
//  g++ -std=c++17 -O2 -o minesweeper minesweeper_dsa.cpp
//  ./minesweeper
//
//  Requires: C++17 (for structured bindings: auto [r, c] = ...)
//             Standard library: <queue>, <unordered_set>, <vector>
// =============================================================================

# Micromouse Solver

This is a micromouse solver project.
The goal is: given a maze and a starting point in the maze, figure out a path (possibly the 
shortest in number of steps) to the center of the maze.
This is a key part in any micromouse competition.
The second part of the puzzle is interacting with hardware and sensors to physically move
motors and cause a robot to navigate a physical maze.
This latter part is not the focus of this project (for now...).

## Coding Standards
A good set of C coding standards to follow are the [CMU ECE published standards](https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html#shortmethods).
We will take these standards as 'ground truth', and try to stick to these as much as possible.
Where the standard for a particular construct was not defined, we have defined our own standard and 
stayed consistent with it.

### Custom standard extensions
- All comments shall begin with a capital letter (like regular sentences) unless they refer to a physical/logical entity (e.g. filename).
- Casing will follow `snake_case` over `camelCase` or `nocase`.
  For example, we prefer `max_cnt` over `maxCnt` or `maxcnt`.
- 

## References
- Micromouse maze tools: https://github.com/micromouseonline/micromouse_maze_tool
- Ncurses: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
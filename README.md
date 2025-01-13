# editor
This editor was created because I wanted to have a good c++ editor for windows. When I'm on windows writing c++ I find that the main editors people use are VSCode, Visual Studio, CLion, neovim/vim. Personally each one has something that annoys me a bit: 
- VSCode: 

- Visual Studio: 
    - pros:
        - built in debugger
        - intellisense and syntax out of the box
        - free
    - cons:
        - very heavy editor
        - takes up a lot of space
        - tries to enforce its own ecosystem onto you (.sln stuff)
        - multiplayer is sketchy requires logging into microsoft accounts

- CLion: 
    - pros:
        - robust sofware, has a lot of nice features for refactoring
        - good multiplayer support
    - cons:           
        - extremely expensive paid software that requires renewal : (
        - eats up ram quite aggressively and feels heavy to use sometimes

- nvim/vim
    - pros:
        - lightweight
        - very good editing controls
    - cons:
        - learning curve that will cripple your speed until you learn it
        - requires a ton of plugins if you want it to become a c++ ide (LSP, syntax highlighting, etc...)
        - doesn't have native multi-user support (I think someone hacked it in but it requires all parties to know vim controls which could be a reach)

- zed: 
    - pros:
        - robust multi-user support
    - cons:
        - there is no binary available for windows at the moment


The point of this editor is to make a simple and performant editor with multi-user capabilities which sits somewhere between vim and an IDE, but it should run on very low spec computers.


# todo
* first generate the character callback bs, while also creating the insert character function in the viewport, now we have an editor
* visual selection mode
* a viewport can have many buffers, buffers are rectangles where textual data can be stored, they are like recangles, the cursor is only ever in one at a time, and inside of each one you have your own movement commands, also you can't use movement commands to get into another buffer usually, it takes a special command so that you can control the buffer you want on purpose

This line proves we are self hosted!

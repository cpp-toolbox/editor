# editor
This editor was created because I wanted to have a good c++ editor for windows. When I'm on windows writing c++ I find that the main editors people use are VSCode, Visual Studio, CLion, neovim/vim. Personally each one has its own problems: 
* VSCode, Visual Studio: Owned by Microsoft, large over the top editors, multi-user editing is sketchy
* CLion: Paid software, expensive and requires renewal every year
* neovim/vim: Overly complicated setup on windows, no multi-user editing capabilities, if there are they require both parties knowing vim

The point of this editor is to make a simple and performant editor with multi-user capabilities and should be able to integrate with language servers easily.

# todo
* first generate the character callback bs, while also creating the insert character function in the viewport, now we have an editor
* visual selection mode
* a viewport can have many buffers, buffers are rectangles where textual data can be stored, they are like recangles, the cursor is only ever in one at a time, and inside of each one you have your own movement commands, also you can't use movement commands to get into another buffer usually, it takes a special command so that you can control the buffer you want on purpose
This line proves we are self hosted!

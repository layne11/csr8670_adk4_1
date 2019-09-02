# This file is imported from startup.py
# Use it to define global macros

from xide import *

#
# Define your macros as Python functions here

def toggleBookmark():
	doc = app.editor.currentDocument
	doc.moveCursor(mcLineEnd)
	doc.moveCursor(mcLineStart, 1)
	if (len(doc.selectedText) >= 6):
		doc.moveCursor(mcLineEnd)
		doc.moveCursor(mcBackward, 1)
		doc.moveCursor(mcBackward, 1)
		doc.moveCursor(mcBackward, 1)
		doc.moveCursor(mcBackward, 1)
		doc.moveCursor(mcBackward, 1)
		doc.moveCursor(mcBackward, 1)
	if (doc.selectedText == "/*BM*/"):
		doc.cut()
	else:
		doc.moveCursor(mcLineEnd)
		doc.insert("/*BM*/")

def gotoBookmark():
	doc = app.editor.currentDocument
	
	# Determine current line
	doc.moveCursor(mcLineStart)
	doc.moveCursor(mcHome, 1)
	currentLine = doc.selectedText.count("\n") + 1
	doc.gotoLine(currentLine + 1)
	doc.moveCursor(mcLineStart)
	
	# Select all text between cursor and end of file
	doc.moveCursor(mcEnd, 1)
	
	# If it contains a bookmark then goto it
	index = doc.selectedText.find("/*BM*/")
	if (index != -1):
		selectedText = doc.selectedText[:index]
		linesDown = selectedText.count("\n")
		doc.gotoLine(currentLine + linesDown)
		doc.moveCursor(mcDown)
	else:
		doc.gotoLine(currentLine - 1)
		doc.moveCursor(mcHome, 1)
		index = doc.selectedText.find("/*BM*/")
		if (index != -1):
			selectedText = doc.selectedText[:index]
			linesDown = selectedText.count("\n")
			doc.gotoLine(linesDown + 1)
			doc.moveCursor(mcLineStart)
		else:
			doc.gotoLine(currentLine)
			doc.moveCursor(mcLineStart)

def changeCurrentProject():
    numProjects = len(app.projects)

    for proj in range(numProjects):
        if (app.currentProject == app.projects[proj]):
            newProj = (proj + 1) % numProjects
            break

    app.currentProject = app.projects[newProj]
    
def selectedBlockToUpper():
	selectedText = app.editor.currentDocument.selectedText.upper()
	app.editor.currentDocument.insert(selectedText)

def selectedBlockToLower():
	selectedText = app.editor.currentDocument.selectedText.lower()
	app.editor.currentDocument.insert(selectedText)
	
def selectedBlockToggleComment():
	selectedText = app.editor.currentDocument.selectedText
	if (not selectedText):
		app.editor.currentDocument.moveCursor(mcLineStart)
		app.editor.currentDocument.moveCursor(mcLineEnd, 1)
		selectedText = app.editor.currentDocument.selectedText
	strippedText = selectedText
	strippedText = strippedText.strip()
	if (strippedText[0:2] == "/*"):
		selectedText = selectedText.replace("/*", "", 1)
		selectedText = selectedText.replace("*/", "", 1)
		app.editor.currentDocument.insert(selectedText)
	else:
		app.editor.currentDocument.insert("/*" + selectedText + "*/")
    
def openForEdit():
	import os
	call_this = "p4 open " + app.editor.currentDocument.path
	os.system(call_this)
    
    
# Clear all macros to prevent duplicates if you reload this file
app.clearMacros()

# Add macros to the IDE here by calling app.addMacro
# e.g.
app.addMacro(toggleBookmark, "Toggle bookmark", "Creates or removes a bookmark on the cursor's line", "Ctrl+F2")
app.addMacro(gotoBookmark, "Goto bookmark", "Moves the cursor to the next bookmark in the current file", "F2")
app.addMacro(changeCurrentProject, "Change current project", "Swaps active project", "Ctrl+Q")
app.addMacro(selectedBlockToUpper, "To uppercase", "Makes selected block uppercase", "Ctrl+Shift+U") 
app.addMacro(selectedBlockToLower, "To lowercase", "Makes selected block lowercase", "Ctrl+U") 
app.addMacro(selectedBlockToggleComment, "Toggle comment", "Comment/Uncomment the selected block or current line", "Alt+/")
app.addMacro(openForEdit, "Open for edit", "Executes 'p4 open <filename>' for the current file", "Ctrl+Shift+0")

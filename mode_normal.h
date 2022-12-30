void txmotion_c_nl_indent(Text tx, int *px, int *py)
{
	int x, y;
	int sp;
	Line line;
	char *buf;
	int len;
	int tabs;

	x = tx->curX;
	y = tx->curY;

	// get spacing of current line	
	line = tx->lines + y;
	for(len = line->len, buf = line->buf; len; len--, buf++)
	{
		if(*buf == '\t')
			sp += 4 - sp % 4;
		else if(*buf == ' ')
			sp++;
	}
	// give the next line the same spacing
	_txbreak(tx);
	// break modifies these
	x = tx->curX;
	y = tx->curY;
	tabs = sp / 4;
	if(tabs)
	{
		char buf[tabs];
		memset(buf, '\t', tabs);
		line = tx->lines + y;
		_txinsertnstr(line, 0, buf, tabs);
	}

	*px = x;
	*py = y;
}

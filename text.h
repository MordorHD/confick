Text txcreate(int initLineCap, int x, int y, int width, int height)
{
	Text tx;

	tx = malloc(sizeof(*tx));
	tx->flags = 0;
	tx->state = 0;
	tx->scrollX = 0;
	tx->scrollY = 0;
	tx->x = x;
	tx->y = y;
	tx->width = width;
	tx->height = height;
	tx->curX = 0;
	tx->curY = 0;
	initLineCap = initLineCap < 1 ? 1 : initLineCap;
	tx->lines = malloc(initLineCap * sizeof*tx->lines);
	tx->lineCap = initLineCap;
	tx->lineCnt = 1;
	memset(tx->lines, 0, sizeof*tx->lines);
	return tx;
}

void txfree(Text tx)
{
	for(int i = 0; i < tx->lineCnt; i++)
		free(tx->lines[i].buf);
	free(tx->lines);
	free(tx);
}

void txkey(Text tx, int key)
{
	switch(key)
	{
	case KEY_LEFT: txmotion(tx, TXMOTION_LEFT); break;
	case KEY_UP: txmotion(tx, TXMOTION_UP); break;
	case KEY_RIGHT: txmotion(tx, TXMOTION_RIGHT); break;
	case KEY_DOWN: txmotion(tx, TXMOTION_DOWN); break;
	case KEY_BACKSPACE: 
		if(!tx->curX)
		{
			if(!tx->curY)
			{
				txmove(tx, 0, 0);
				break;
			}
			tx->curY--;
			tx->curX = tx->lines[tx->curY].len;
		}
		else
			tx->curX--;
	case 330: txdelc(tx); break;
	case 'q':
		discard();
		break;
	default:
		//_txgrow(tx);
		//tx->lineCnt++;
		if(key <= 255)
			txputc(tx, key);
	}
}

void txdraw(Text tx)
{	
	// this buffer holds the line number
	char buf[(8 * sizeof(int) - 1) / 3 + 2];
	for(int i = 0; i <= tx->height; i++)
	{
		int y = i + tx->scrollY;
		snprintf(buf, sizeof(buf), "%d", y + 1);
		if(y >= tx->lineCnt)
		{
			attron(COLOR_PAIR(2));
			mvaddch(i, tx->x - 2, '~');
			attroff(COLOR_PAIR(2));
		}
		else
		{
			// number of visible chars
			int visCh = tx->lines[y].len - tx->scrollX;
			attron(COLOR_PAIR(1));
			mvaddstr(i + tx->y, tx->x - 1 - strlen(buf), buf);
			attroff(COLOR_PAIR(1));
			if(visCh > 0)
				mvaddnstr(i + tx->y, tx->x, tx->lines[y].buf + tx->scrollX, min(visCh, tx->width + 1));
		}
		mvaddch(i + tx->y, tx->x + tx->width + 1, '#');
	}
	for(int i = 0; i <= tx->width; i++)
		mvaddch(tx->y + tx->height + 1, tx->x + i, '#');
	move(tx->curY + tx->y - tx->scrollY, tx->curX + tx->x - tx->scrollX);

}

void txmove(Text tx, int x, int y)
{
	int sx, sy; // scrolled x and y

	assert(x >= 0 && y >= 0 && y < tx->lineCnt && x <= tx->lines[y].len && "txmove > out of bounds");
	tx->curX = x;
	tx->curY = y;
	// make caret visible if it is outside
	sx = x - tx->scrollX;
	sy = y - tx->scrollY;
	if(sx < 0)
		tx->scrollX = max(x - 4, 0);
	else if(sx > tx->width)
		tx->scrollX = x - tx->width + 3;
	// same for y
	if(sy < 0)
		tx->scrollY = y;
	else if(sy > tx->height)
		tx->scrollY = y - tx->height;
}

void txmotion(Text tx, int motion)
{
	int x, y;

	x = tx->curX;
	y = tx->curY;
	switch(motion)
	{
	case TXMOTION_LEFT:
		if(!x)
		{
			if(!y)
				break;
			y--;
			x = tx->lines[y].len;
			break;
		}
		x--;
		break;
	case TXMOTION_UP:
		if(!y)
			break;
		y--;
		x = min(x, tx->lines[y].len);
		break;
	case TXMOTION_RIGHT:
		if(x == tx->lines[y].len)
		{
			if(y + 1 == tx->lineCnt)
				break;
			y++;
			x = 0;
			break;
		}
		x++;
		break;
	case TXMOTION_DOWN: 
		if(y + 1 == tx->lineCnt)
			break;
		y++;
		x = min(x, tx->lines[y].len);
		break;
	}
	txmove(tx, x, y);
}

void _txgrow(Text tx)
{
	if(tx->lineCnt + 1 > tx->lineCap)
	{
		tx->lineCap *= 2;
		tx->lineCap++;
		tx->lines = realloc(tx->lines, tx->lineCap * sizeof*tx->lines);
	}
}

void _txbreak(Text tx)
{
	Line line, nextLine;
	int breakLen;
	
	_txgrow(tx);
	line = txline(tx);
	nextLine = line + 1;	
	memmove(nextLine, line, (tx->lineCnt - 1 - tx->curY) * sizeof*line);
	if((breakLen = line->len - tx->curX))
	{
		nextLine->buf = malloc(breakLen);
		nextLine->cap = nextLine->len = breakLen;
		line->len -= breakLen;
		memmove(nextLine->buf, line->buf + line->len, breakLen);
	}
	else
	{
		nextLine->buf = NULL;
		nextLine->cap = nextLine->len = 0;
	}
	tx->lineCnt++;
	tx->curX = 0;
	tx->curY++;
}

void _txgrowline(Line line, int to)
{
	int delta = to - line->cap;
	if(delta > 0)
	{
		line->cap *= 2;
		line->cap += delta;
		line->buf = realloc(line->buf, line->cap);
	}
}

void _txinsertchar(Line line, int index, int c)
{
	char *dest;	

	_txgrowline(line, line->len + 1);
	dest = line->buf + index;
	memmove(dest + 1, dest, line->len - index);
	line->len++;
	*dest = c;
}

void txputc(Text tx, int c)
{
	Line line;

	if(c == '\r' || c == '\n')
	{
		_txbreak(tx);
	}
	else
	{
		line = txline(tx);
		_txinsertchar(line, tx->curX, c);
		mvaddch(tx->curY + tx->y - tx->scrollY, tx->curX + tx->x - tx->scrollX, c);
		tx->curX++;
	}
	// update cursor
	txmove(tx, tx->curX, tx->curY);
}

void _txinsertnstr(Line line, int index, const char *s, int n)
{
	char *dest;

	_txgrowline(line, line->len + n);
	dest = line->buf + index;
	memmove(dest + n, dest, line->len - index);
	line->len += n;
	memcpy(dest, s, n);
}

void txdelc(Text tx)
{
	Line line;

	line = txline(tx);
	if(tx->curX == line->len)
	{
		// bring the next line up to this one
		if(tx->curY + 1 != tx->lineCnt)
		{
			Line nextLine = line + 1;
			_txinsertnstr(line, line->len, nextLine->buf, nextLine->len);
			free(nextLine->buf);
			tx->lineCnt--;
			memmove(nextLine, nextLine + 1, (tx->lineCnt - tx->curY) * sizeof*line);
		}
	}
	else
	{
		// remove char 
		char *dest = line->buf + tx->curX;
		line->len--;
		memmove(dest, dest + 1, line->len - tx->curX);
	}
	// update caret
	txmove(tx, tx->curX, tx->curY);
}

const char *_txlinesep(const char *s)
{
	for(; *s; s++)
	{
		if(*s == '\r' || *s == '\n')
			return s;
	}
	return NULL;
}

int _txlineseplen(const char *s)
{
	return 1 + (s[0] == '\r' && s[1] == '\n');
}

void txputs(Text tx, const char *s)
{
	const char *e;
	int l;

	while(1)
	{
		e = _txlinesep(s);
		l = e ? e - s : strlen(s);
		_txinsertnstr(txline(tx), tx->curX, s, l);
		mvaddnstr(tx->curY + tx->x - tx->scrollY, tx->curX + tx->y - tx->scrollX, s, l);
		tx->curX += l;
		if(!e)
			break;
		_txbreak(tx);
		s = e + _txlineseplen(e);
	}
	// update cursor
	txmove(tx, tx->curX, tx->curY);
}
